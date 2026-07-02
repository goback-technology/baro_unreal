// Fill out your copyright notice in the Description page of Project Settings.

#include "MjpegStreamServer.h"

#include "Common/TcpListener.h"   // FTcpListener (pulls SocketSubsystem/IPv4Endpoint/RunnableThread)
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "HAL/RunnableThread.h"
#include "Misc/ScopeLock.h"

DEFINE_LOG_CATEGORY_STATIC(LogMjpegStream, Log, All);

namespace
{
	const TCHAR* const kBoundary = TEXT("baroframe");
}

FMjpegStreamServer::FMjpegStreamServer()
{
}

FMjpegStreamServer::~FMjpegStreamServer()
{
	StopServer();
}

bool FMjpegStreamServer::StartServer(int32 Port, int32 InFps)
{
	Fps = FMath::Clamp(InFps, 1, 60);
	bStop = false;

	const FIPv4Endpoint Endpoint(FIPv4Address::Any, static_cast<uint16>(Port));
	// 짧은 폴링 간격으로 accept 응답성 확보.
	Listener = new FTcpListener(Endpoint, FTimespan::FromMilliseconds(200));
	Listener->OnConnectionAccepted().BindRaw(this, &FMjpegStreamServer::HandleConnection);

	Thread = FRunnableThread::Create(this, TEXT("HucomsMjpegStream"), 0, TPri_Normal);
	if (!Thread)
	{
		UE_LOG(LogMjpegStream, Error, TEXT("[MJPEG] 워커 스레드 생성 실패"));
		StopServer();
		return false;
	}

	UE_LOG(LogMjpegStream, Log, TEXT("[MJPEG] 연속 스트림 서버 시작 :%d (fps=%d)"), Port, Fps);
	return true;
}

void FMjpegStreamServer::StopServer()
{
	bStop = true;

	// 리스너 먼저 정지(신규 accept 차단). 소멸자가 accept 스레드 kill + listen 소켓 정리.
	if (Listener)
	{
		delete Listener;
		Listener = nullptr;
	}

	if (Thread)
	{
		Thread->WaitForCompletion();
		delete Thread;
		Thread = nullptr;
	}

	CloseAllClients();
}

bool FMjpegStreamServer::HandleConnection(FSocket* Socket, const FIPv4Endpoint& Endpoint)
{
	if (!Socket)
	{
		return false;
	}
	Socket->SetNonBlocking(false);
	{
		FScopeLock P(&PendingLock);
		Pending.Add(Socket);
	}
	UE_LOG(LogMjpegStream, Log, TEXT("[MJPEG] 클라이언트 연결: %s"), *Endpoint.ToString());
	return true; // 소유권 인수
}

void FMjpegStreamServer::UpdateFrame(const TArray<uint8>& Jpeg)
{
	FScopeLock F(&FrameLock);
	LatestFrame = Jpeg;
}

bool FMjpegStreamServer::HasClients() const
{
	{
		FScopeLock C(&ClientsLock);
		if (Clients.Num() > 0) { return true; }
	}
	{
		FScopeLock P(&PendingLock);
		if (Pending.Num() > 0) { return true; }
	}
	return false;
}

bool FMjpegStreamServer::SendAll(FSocket* S, const uint8* Data, int32 Len)
{
	int32 Total = 0;
	while (Total < Len)
	{
		int32 Sent = 0;
		if (!S->Send(Data + Total, Len - Total, Sent) || Sent <= 0)
		{
			return false;
		}
		Total += Sent;
	}
	return true;
}

void FMjpegStreamServer::DestroyClientSocket(FSocket* S)
{
	if (!S)
	{
		return;
	}
	S->Close();
	if (ISocketSubsystem* SS = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM))
	{
		SS->DestroySocket(S);
	}
}

void FMjpegStreamServer::CloseAllClients()
{
	{
		FScopeLock C(&ClientsLock);
		for (FClient& Cl : Clients) { DestroyClientSocket(Cl.Socket); }
		Clients.Reset();
	}
	{
		FScopeLock P(&PendingLock);
		for (FSocket* S : Pending) { DestroyClientSocket(S); }
		Pending.Reset();
	}
}

uint32 FMjpegStreamServer::Run()
{
	const FString HeaderStr = FString::Printf(
		TEXT("HTTP/1.1 200 OK\r\n")
		TEXT("Content-Type: multipart/x-mixed-replace; boundary=%s\r\n")
		TEXT("Cache-Control: no-cache\r\n")
		TEXT("Pragma: no-cache\r\n")
		TEXT("Connection: close\r\n\r\n"),
		kBoundary);

	const float SleepSec = 1.f / FMath::Max(1, Fps);

	while (!bStop)
	{
		// 1) 대기열 -> 활성 (중첩 락 없이)
		TArray<FSocket*> NewOnes;
		{
			FScopeLock P(&PendingLock);
			NewOnes = MoveTemp(Pending);
			Pending.Reset();
		}
		if (NewOnes.Num() > 0)
		{
			FScopeLock C(&ClientsLock);
			for (FSocket* S : NewOnes) { Clients.Add(FClient{ S, false }); }
		}

		// 2) 최신 프레임 스냅샷
		TArray<uint8> Frame;
		{
			FScopeLock F(&FrameLock);
			Frame = LatestFrame;
		}

		// 3) 각 클라이언트에 송신
		if (Frame.Num() > 0)
		{
			const FString PartHdr = FString::Printf(
				TEXT("--%s\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n"),
				kBoundary, Frame.Num());

			FScopeLock C(&ClientsLock);
			for (int32 i = Clients.Num() - 1; i >= 0; --i)
			{
				FClient& Cl = Clients[i];
				bool bOk = true;

				if (!Cl.bHeaderSent)
				{
					FTCHARToUTF8 H(*HeaderStr);
					bOk = SendAll(Cl.Socket, reinterpret_cast<const uint8*>(H.Get()), H.Length());
					Cl.bHeaderSent = true;
				}
				if (bOk)
				{
					FTCHARToUTF8 PH(*PartHdr);
					bOk = SendAll(Cl.Socket, reinterpret_cast<const uint8*>(PH.Get()), PH.Length());
				}
				if (bOk)
				{
					bOk = SendAll(Cl.Socket, Frame.GetData(), Frame.Num());
				}
				if (bOk)
				{
					const uint8 Trailer[2] = { '\r', '\n' };
					bOk = SendAll(Cl.Socket, Trailer, 2);
				}

				if (!bOk)
				{
					UE_LOG(LogMjpegStream, Log, TEXT("[MJPEG] 클라이언트 연결 종료(송신 실패) -> 정리"));
					DestroyClientSocket(Cl.Socket);
					Clients.RemoveAt(i);
				}
			}
		}

		// 4) Fps 페이싱
		FPlatformProcess::Sleep(SleepSec);
	}

	return 0;
}

void FMjpegStreamServer::Stop()
{
	bStop = true;
}

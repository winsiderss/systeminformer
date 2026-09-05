# System Informer 기술 가이드북

---

# 전자책 (E-Book)

**Windows 시스템 모니터링 도구의 설계와 구현**

---

## 저자 정보
- **프로젝트**: System Informer (구 Process Hacker)
- **라이선스**: MIT License
- **발행일**: 2026년 1월

---

# 목차

1. [서론](#1-서론)
2. [프로젝트 구조](#2-프로젝트-구조)
3. [핵심 기능](#3-핵심-기능)
4. [기술 스택](#4-기술-스택)
5. [빌드 환경](#5-빌드-환경)
6. [핵심 기술 구현](#6-핵심-기술-구현)
7. [플러그인 개발](#7-플러그인-개발)
8. [보안 메커니즘](#8-보안-메커니즘)
9. [부록](#9-부록)

---

# 1. 서론

## 1.1 System Informer란?

System Informer는 Windows 운영체제를 위한 고급 시스템 모니터링 및 분석 도구입니다. 기존 Windows 작업 관리자의 한계를 넘어 프로세스, 스레드, 핸들, 메모리, 네트워크 연결 등 시스템의 모든 측면을 심층적으로 분석할 수 있습니다.

## 1.2 주요 특징

```
┌─────────────────────────────────────────────────────────────┐
│                    System Informer 특징                      │
├─────────────────────────────────────────────────────────────┤
│ ✓ 상세한 프로세스/스레드/핸들 정보                           │
│ ✓ 실시간 CPU/메모리/디스크/네트워크 모니터링                 │
│ ✓ 커널 드라이버를 통한 심층 시스템 접근                      │
│ ✓ 악성코드 분석을 위한 고급 기능                             │
│ ✓ 확장 가능한 플러그인 시스템                                │
│ ✓ 100% 오픈소스 (MIT 라이선스)                               │
└─────────────────────────────────────────────────────────────┘
```

## 1.3 역사

- **2008**: Process Hacker로 시작
- **2022**: System Informer로 리브랜딩
- **현재**: 940,000+ 라인의 코드베이스

---

# 2. 프로젝트 구조

## 2.1 아키텍처 개요

System Informer는 3계층 아키텍처를 따릅니다:

```
┌─────────────────────────────────────────┐
│          사용자 인터페이스 계층          │
│    SystemInformer.exe + Plugins         │
├─────────────────────────────────────────┤
│            핵심 라이브러리 계층           │
│         phlib.dll + kphlib.dll          │
├─────────────────────────────────────────┤
│            커널 드라이버 계층            │
│           KSystemInformer.sys           │
└─────────────────────────────────────────┘
```

## 2.2 디렉토리 구조

| 디렉토리 | 설명 | 주요 파일 |
|----------|------|-----------|
| `SystemInformer/` | GUI 애플리케이션 | mainwnd.c, proctree.c |
| `phlib/` | 핵심 라이브러리 | native.c, treenew.c |
| `phnt/` | NT API 헤더 | ntpsapi.h, ntmmapi.h |
| `KSystemInformer/` | 커널 드라이버 | driver.c, comms.c |
| `kphlib/` | KPH 통신 라이브러리 | kphcomms.c |
| `plugins/` | 플러그인 (11개) | 각 플러그인 폴더 |

## 2.3 컴포넌트 통신

```
User Mode                    Kernel Mode
┌────────────┐              ┌─────────────────┐
│SystemInform│  DeviceIO    │ KSystemInformer │
│   er.exe   │◄────────────►│     .sys        │
└─────┬──────┘              └────────┬────────┘
      │                              │
      ▼                              ▼
┌────────────┐              ┌─────────────────┐
│  phlib.dll │              │  Windows Kernel │
└────────────┘              └─────────────────┘
```

---

# 3. 핵심 기능

## 3.1 프로세스 관리

### 프로세스 목록
- 실시간 프로세스 정보 표시
- 트리/플랫 뷰 전환
- 상세 속성 (토큰, 모듈, 핸들 등)

### 프로세스 작업
| 작업 | 설명 |
|------|------|
| Terminate | 프로세스 종료 |
| Suspend/Resume | 일시 중지/재개 |
| Create Dump | 메모리 덤프 생성 |
| Inject DLL | DLL 주입 |

## 3.2 시스템 모니터링

### 실시간 그래프
- CPU 사용률 (코어별)
- 메모리 사용량
- 디스크 I/O
- 네트워크 트래픽
- GPU 사용률 (플러그인)

### 정보 수집 주기
```c
// 기본 업데이트 간격: 1초
#define PH_PROVIDER_UPDATE_INTERVAL 1000
```

## 3.3 네트워크 분석

- TCP/UDP 연결 목록
- 로컬/원격 주소 및 포트
- 연결 상태 (Established, Listening 등)
- 프로세스별 네트워크 사용량

## 3.4 서비스 관리

- 서비스 시작/중지/재시작
- 서비스 속성 편집
- 의존성 분석 (플러그인)
- 복구 옵션 설정 (플러그인)

---

# 4. 기술 스택

## 4.1 프로그래밍 언어

```
C 언어 ████████████████████████████░░ 95%
C++    ██░░░░░░░░░░░░░░░░░░░░░░░░░░░░  4%
기타    █░░░░░░░░░░░░░░░░░░░░░░░░░░░░░  1%
```

## 4.2 Windows API 계층

| API | 용도 | 예시 |
|-----|------|------|
| Win32 | GUI, 일반 작업 | CreateWindow, ReadFile |
| Native NT | 저수준 시스템 | NtQuerySystemInformation |
| WDM | 커널 드라이버 | IoCreateDevice |
| ETW | 이벤트 추적 | StartTrace |
| WMI | 시스템 관리 | Win32_Process |
| COM/DirectX | GPU 정보 | D3DKMTQueryStatistics |

## 4.3 써드파티 라이브러리

| 라이브러리 | 용도 |
|------------|------|
| JSON-C | JSON 파싱 |
| PCRE | 정규 표현식 |
| Zydis | 디스어셈블러 |
| MaxMindDB | GeoIP |
| Detours | 함수 후킹 |
| MinIZ | ZIP 압축 |

---

# 5. 빌드 환경

## 5.1 필수 요구사항

```
☑ Visual Studio 2022
☑ Windows 10/11 SDK (10.0.22621.0)
☑ C++ ATL/MFC 구성 요소
☑ Git
```

## 5.2 드라이버 빌드 시 추가 요구사항

```
☑ Windows Driver Kit (WDK)
☑ 테스트 서명 모드 또는 EV 인증서
```

## 5.3 빌드 명령

```cmd
:: 릴리스 빌드
msbuild SystemInformer.sln /p:Configuration=Release /p:Platform=x64

:: 플러그인 빌드
msbuild Plugins.sln /p:Configuration=Release /p:Platform=x64
```

## 5.4 CMake 빌드

```bash
mkdir build && cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

---

# 6. 핵심 기술 구현

## 6.1 Native NT API 활용

System Informer는 비공개 NT API를 광범위하게 사용합니다:

```c
// 프로세스 열거
NTSTATUS PhEnumProcesses(_Out_ PVOID *Processes)
{
    return NtQuerySystemInformation(
        SystemProcessInformation,
        buffer,
        bufferSize,
        &returnLength
    );
}
```

### 주요 NT API 함수

| 함수 | 용도 |
|------|------|
| NtQuerySystemInformation | 시스템 정보 조회 |
| NtQueryInformationProcess | 프로세스 정보 |
| NtReadVirtualMemory | 메모리 읽기 |
| NtQueryObject | 객체 정보 |

## 6.2 참조 카운팅 메모리 관리

```c
// 객체 생성
PPH_STRING string = PhCreateString(L"Hello");

// 참조 증가
PhReferenceObject(string);

// 참조 감소 (0이면 자동 해제)
PhDereferenceObject(string);
```

## 6.3 커널 드라이버 통신

```c
// 드라이버 연결
NTSTATUS KphConnect(_Out_ PKPH_COMMS_HANDLE Handle)
{
    return NtOpenFile(
        &Handle->DeviceHandle,
        FILE_READ_DATA | FILE_WRITE_DATA,
        &objectAttributes,
        &ioStatusBlock,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        0
    );
}
```

## 6.4 TreeNew 고성능 컨트롤

가상화된 트리/리스트 컨트롤로 수천 개 항목을 효율적으로 표시:

```c
// 노드 콜백 (필요할 때만 데이터 요청)
BOOLEAN TreeNewCallback(
    _In_ PH_TREENEW_MESSAGE Message,
    _In_ PVOID Parameter1,
    _In_ PVOID Parameter2
)
{
    switch (Message) {
        case TreeNewGetCellText:
            // 셀 텍스트 반환
            break;
        case TreeNewGetChildren:
            // 자식 노드 반환
            break;
    }
}
```

## 6.5 ETW 이벤트 추적

```c
// ETW 세션 시작
StartTrace(&sessionHandle, SessionName, &properties);

// 공급자 활성화
EnableTraceEx2(
    sessionHandle,
    &DiskIoGuid,
    EVENT_CONTROL_CODE_ENABLE_PROVIDER,
    TRACE_LEVEL_INFORMATION,
    0, 0, 0, NULL
);
```

## 6.6 동적 데이터 시스템

Windows 버전별 커널 구조체 오프셋 관리:

```xml
<!-- kphdyn.xml -->
<Version Build="22621">
  <EPROCESS>
    <UniqueProcessId>0x440</UniqueProcessId>
    <Token>0x4b8</Token>
  </EPROCESS>
</Version>
```

---

# 7. 플러그인 개발

## 7.1 플러그인 구조

```c
// 필수 내보내기 함수
BOOLEAN PhPluginLoad(
    _In_ PPH_PLUGIN Plugin,
    _In_ ULONG SystemServiceNumber
)
{
    // 플러그인 초기화
    PH_PLUGIN_INFORMATION info;
    info.Name = L"MyPlugin";
    info.Author = L"Author";
    info.Description = L"Description";

    PhRegisterPlugin(&info);

    // 콜백 등록
    PhRegisterCallback(
        &PhProcessAddedEvent,
        OnProcessAdded,
        NULL,
        &CallbackRegistration
    );

    return TRUE;
}
```

## 7.2 내장 플러그인

| 플러그인 | 기능 |
|----------|------|
| DotNetTools | .NET 런타임 분석 |
| ExtendedTools | ETW, GPU, NPU 모니터링 |
| HardwareDevices | 디스크, 네트워크 모니터링 |
| NetworkTools | Ping, Traceroute, GeoIP |
| OnlineChecks | VirusTotal 검사 |
| Updater | 자동 업데이트 |

## 7.3 확장 가능한 지점

- 메인 메뉴 항목 추가
- 컨텍스트 메뉴 확장
- 프로세스 속성 페이지 추가
- 컬럼 추가
- 정보 제공자 등록

---

# 8. 보안 메커니즘

## 8.1 서명 검증

```c
// Authenticode 서명 검증
VERIFY_RESULT PhVerifyFile(_In_ PWSTR FileName)
{
    WINTRUST_DATA trustData;
    // WinVerifyTrust API 사용
    return WinVerifyTrust(NULL, &actionGuid, &trustData);
}
```

## 8.2 드라이버 클라이언트 검증

```c
// 클라이언트 프로세스 검증
NTSTATUS KphVerifyClient(_In_ PEPROCESS Process)
{
    // 이미지 해시 계산 후 신뢰할 수 있는 해시와 비교
    if (RtlEqualMemory(hash, KphTrustedHash, sizeof(hash))) {
        return STATUS_SUCCESS;
    }
    return STATUS_ACCESS_DENIED;
}
```

## 8.3 보안 기능 활성화

빌드 시 SDL (Security Development Lifecycle) 검사 활성화:

```xml
<PropertyGroup>
  <SDLCheck>true</SDLCheck>
  <TreatWarningAsError>true</TreatWarningAsError>
</PropertyGroup>
```

---

# 9. 부록

## 9.1 프로젝트 통계

| 항목 | 수치 |
|------|------|
| 총 소스 파일 | 864개 |
| 코드 라인 | 940,000+ |
| C 파일 | 504개 |
| 헤더 파일 | 335개 |
| 플러그인 수 | 11개 |

## 9.2 주요 소스 파일

| 파일 | 위치 | 기능 |
|------|------|------|
| proctree.c | SystemInformer/ | 프로세스 트리 |
| procprv.c | SystemInformer/ | 프로세스 공급자 |
| native.c | phlib/ | Native API 래퍼 |
| treenew.c | phlib/ | TreeNew 컨트롤 |
| comms.c | KSystemInformer/ | 드라이버 통신 |

## 9.3 참고 자료

### 공식 리소스
- GitHub: github.com/winsiderss/si
- 문서: docs.systeminformer.com

### Windows 내부 구조
- Windows Internals (7th Edition)
- ReactOS 소스 코드

### Native API
- phnt 헤더 컬렉션
- ntinternals.net

## 9.4 용어 사전

| 용어 | 설명 |
|------|------|
| EPROCESS | 커널의 프로세스 구조체 |
| ETHREAD | 커널의 스레드 구조체 |
| PEB | Process Environment Block |
| TEB | Thread Environment Block |
| NT API | Native NT API |
| WDM | Windows Driver Model |
| ETW | Event Tracing for Windows |
| PPL | Protected Process Light |

---

# 라이선스

```
MIT License

Copyright (c) System Informer Authors

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software.
```

---

**문서 끝**

*이 전자책은 System Informer 프로젝트의 기술 문서를 종합하여 제작되었습니다.*

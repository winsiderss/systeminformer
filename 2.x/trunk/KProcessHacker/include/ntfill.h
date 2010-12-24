#ifndef NTFILL_H
#define NTFILL_H

// Processes

typedef NTSTATUS (NTAPI *_PsSuspendProcess)(
    __in PEPROCESS Process
    );

typedef NTSTATUS (NTAPI *_PsResumeProcess)(
    __in PEPROCESS Process
    );

typedef struct _EJOB *PEJOB;

extern POBJECT_TYPE *PsJobType;

NTKERNELAPI
PEJOB
NTAPI
PsGetProcessJob(
    __in PEPROCESS Process
    );

#endif

/*
 * Process Hacker .NET Tools -
 *   .NET Performance property page
 *
 * Copyright (C) 2011-2015 wj32
 * Copyright (C) 2015 dmex
 *
 * This file is part of Process Hacker.
 *
 * Process Hacker is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Process Hacker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Process Hacker.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "dn.h"
#include "clr\perfcounterdefs.h"

typedef struct _PERFPAGE_CONTEXT
{
    HWND WindowHandle;
    HWND AppDomainsLv;
    HWND CountersLv;
    PPH_PROCESS_ITEM ProcessItem;

    union
    {
        BOOLEAN Flags;
        struct
        {
            BOOLEAN Enabled : 1;
            BOOLEAN ControlBlockValid : 1;
            BOOLEAN ClrV4 : 1;
            BOOLEAN IsWow64 : 1;
            BOOLEAN Spare : 4;
        };
    };

    HANDLE ProcessHandle;
    HANDLE BlockTableHandle;
    PVOID BlockTableAddress;
    PH_CALLBACK_REGISTRATION ProcessesUpdatedCallbackRegistration;
} PERFPAGE_CONTEXT, *PPERFPAGE_CONTEXT;

typedef enum _DOTNET_CATEGORY
{
    // .NET CLR Exceptions (Runtime statistics on CLR exception handling)
    DOTNET_CATEGORY_EXCEPTIONS,
    // .NET CLR Interop (Stats for CLR interop)
    DOTNET_CATEGORY_INTEROP,
    // .NET CLR Jit (Stats for CLR Jit)
    DOTNET_CATEGORY_JIT,
    // .NET CLR Loading (Statistics for CLR Class Loader)
    DOTNET_CATEGORY_LOADING,
    // .NET CLR LocksAndThreads (Stats for CLR Locks and Threads)
    DOTNET_CATEGORY_LOCKSANDTHREADS,
    // .NET CLR Memory (Counters for CLR Garbage Collected heap)
    DOTNET_CATEGORY_MEMORY,
    // .NET CLR Remoting (Stats for CLR Remoting)
    DOTNET_CATEGORY_REMOTING,
    // .NET CLR Security (Stats for CLR Security)
    DOTNET_CATEGORY_SECURITY
} DOTNET_CATEGORY;

typedef enum _DOTNET_INDEX
{
    DOTNET_INDEX_EXCEPTIONS_THROWNCOUNT,
    DOTNET_INDEX_EXCEPTIONS_FILTERSCOUNT,
    DOTNET_INDEX_EXCEPTIONS_FINALLYCOUNT,

    DOTNET_INDEX_INTEROP_CCWCOUNT,
    DOTNET_INDEX_INTEROP_STUBCOUNT,
    DOTNET_INDEX_INTEROP_MARSHALCOUNT,
    DOTNET_INDEX_INTEROP_TLBIMPORTPERSEC,
    DOTNET_INDEX_INTEROP_TLBEXPORTPERSEC,

    DOTNET_INDEX_JIT_ILMETHODSJITTED,
    DOTNET_INDEX_JIT_ILBYTESJITTED,
    DOTNET_INDEX_JIT_ILTOTALBYTESJITTED,
    DOTNET_INDEX_JIT_FAILURES,
    DOTNET_INDEX_JIT_TIME,

    DOTNET_INDEX_LOADING_CURRENTLOADED,
    DOTNET_INDEX_LOADING_TOTALLOADED,
    DOTNET_INDEX_LOADING_CURRENTAPPDOMAINS,
    DOTNET_INDEX_LOADING_TOTALAPPDOMAINS,
    DOTNET_INDEX_LOADING_CURRENTASSEMBLIES,
    DOTNET_INDEX_LOADING_TOTALASSEMBLIES,
    DOTNET_INDEX_LOADING_ASSEMBLYSEARCHLENGTH,
    DOTNET_INDEX_LOADING_TOTALLOADFAILURES,
    DOTNET_INDEX_LOADING_BYTESINLOADERHEAP,
    DOTNET_INDEX_LOADING_TOTALAPPDOMAINSUNLOADED,

    DOTNET_INDEX_LOCKSANDTHREADS_TOTALLOCKS,
    DOTNET_INDEX_LOCKSANDTHREADS_TOTALQUEUELENGTH,
    DOTNET_INDEX_LOCKSANDTHREADS_QUEUELENGTHPEAK,
    DOTNET_INDEX_LOCKSANDTHREADS_CURRENTLOGICAL,
    DOTNET_INDEX_LOCKSANDTHREADS_CURRENTPHYSICAL,
    DOTNET_INDEX_LOCKSANDTHREADS_CURRENTRECOGNIZED,
    DOTNET_INDEX_LOCKSANDTHREADS_TOTALRECOGNIZED,

    DOTNET_INDEX_MEMORY_GENZEROCOLLECTIONS,
    DOTNET_INDEX_MEMORY_GENONECOLLECTIONS,
    DOTNET_INDEX_MEMORY_GENTWOCOLLECTIONS,
    DOTNET_INDEX_MEMORY_PROMOTEDFROMGENZERO,
    DOTNET_INDEX_MEMORY_PROMOTEDFROMGENONE,
    DOTNET_INDEX_MEMORY_PROMOTEDFINALFROMGENZERO,
    DOTNET_INDEX_MEMORY_PROCESSID,
    DOTNET_INDEX_MEMORY_GENZEROHEAPSIZE,
    DOTNET_INDEX_MEMORY_GENONEHEAPSIZE,
    DOTNET_INDEX_MEMORY_GENTWOHEAPSIZE,
    DOTNET_INDEX_MEMORY_LOHSIZE,
    DOTNET_INDEX_MEMORY_FINALSURVIVORS,
    DOTNET_INDEX_MEMORY_GCHANDLES,
    DOTNET_INDEX_MEMORY_INDUCEDGC,
    DOTNET_INDEX_MEMORY_TIMEINGC,
    DOTNET_INDEX_MEMORY_BYTESINALLHEAPS,
    DOTNET_INDEX_MEMORY_TOTALCOMMITTED,
    DOTNET_INDEX_MEMORY_TOTALRESERVED,
    DOTNET_INDEX_MEMORY_TOTALPINNED,
    DOTNET_INDEX_MEMORY_TOTALSINKS,
    DOTNET_INDEX_MEMORY_TOTALBYTESSINCESTART,
    DOTNET_INDEX_MEMORY_TOTALLOHBYTESSINCESTART,

    DOTNET_INDEX_REMOTING_TOTALREMOTECALLS,
    DOTNET_INDEX_REMOTING_CHANNELS,
    DOTNET_INDEX_REMOTING_CONTEXTPROXIES,
    DOTNET_INDEX_REMOTING_CONTEXTCLASSESLOADED,
    DOTNET_INDEX_REMOTING_CONTEXTS,
    DOTNET_INDEX_REMOTING_CONTEXTSALLOCATED,

    DOTNET_INDEX_SECURITY_TOTALRUNTIMECHECKS,
    DOTNET_INDEX_SECURITY_LINKTIMECHECKS,
    DOTNET_INDEX_SECURITY_TIMEINRTCHECKS,
    DOTNET_INDEX_SECURITY_STACKWALKDEPTH
} DOTNET_INDEX;

PWSTR DotNetCategoryStrings[] =
{
    L".NET CLR Exceptions",
    L".NET CLR Interop",
    L".NET CLR Jit",
    L".NET CLR Loading",
    L".NET CLR LocksAndThreads",
    L".NET CLR Memory",
    L".NET CLR Remoting",
    L".NET CLR Security"
};

VOID NTAPI DotNetPerfProcessesUpdatedCallback(
    _In_opt_ PVOID Parameter,
    _In_opt_ PVOID Context
    )
{
    PPERFPAGE_CONTEXT context = Context;

    if (context->WindowHandle)
    {
        PostMessage(context->WindowHandle, MSG_UPDATE, 0, 0);
    }
}

VOID DotNetPerfAddListViewGroups(
    _In_ HWND ListViewHandle
    )
{
    ListView_EnableGroupView(ListViewHandle, TRUE);
    PhAddListViewGroup(ListViewHandle, DOTNET_CATEGORY_EXCEPTIONS, L".NET CLR Exceptions");
    PhAddListViewGroup(ListViewHandle, DOTNET_CATEGORY_INTEROP, L".NET CLR Interop");
    PhAddListViewGroup(ListViewHandle, DOTNET_CATEGORY_JIT, L".NET CLR Jit");
    PhAddListViewGroup(ListViewHandle, DOTNET_CATEGORY_LOADING, L".NET CLR Loading");
    PhAddListViewGroup(ListViewHandle, DOTNET_CATEGORY_LOCKSANDTHREADS, L".NET CLR LocksAndThreads");
    PhAddListViewGroup(ListViewHandle, DOTNET_CATEGORY_MEMORY, L".NET CLR Memory");
    PhAddListViewGroup(ListViewHandle, DOTNET_CATEGORY_REMOTING, L".NET CLR Remoting");
    PhAddListViewGroup(ListViewHandle, DOTNET_CATEGORY_SECURITY, L".NET CLR Security");

    // This counter displays the total number of exceptions thrown since the start of the application.
    // These include both .NET exceptions and unmanaged exceptions that get converted into .NET exceptions e.g. null pointer reference exception in unmanaged code
    // would get re-thrown in managed code as a .NET System.NullReferenceException; this counter includes both handled and unhandled exceptions.Exceptions
    // that are re-thrown would get counted again. Exceptions should only occur in rare situations and not in the normal control flow of the program.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_EXCEPTIONS, DOTNET_INDEX_EXCEPTIONS_THROWNCOUNT, L"# of Exceps Thrown", NULL);
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_EXCEPTIONS, DOTNET_INDEX_EXCEPTIONS_FILTERSCOUNT, L"# of Filters Executed", NULL);
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_EXCEPTIONS, DOTNET_INDEX_EXCEPTIONS_FINALLYCOUNT, L"# of Finallys Executed", NULL);

    // Reserved for future use.
    //PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_EXCEPTIONS, , L"Delta from throw to catch site on stack", NULL);

    // # of Exceps Thrown / sec
    // This counter displays the number of exceptions thrown per second.
    // These include both .NET exceptions and unmanaged exceptions that get converted into .NET exceptions e.g.null pointer reference exception in unmanaged code would get
    // re-thrown in managed code as a .NET System.NullReferenceException; this counter includes both handled and unhandled exceptions.
    // Exceptions should only occur in rare situations and not in the normal control flow of the program;
    // this counter was designed as an indicator of potential performance problems due to large (> 100s) rate of exceptions thrown.
    // This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.
    // TODO: We need to count the delta.

    // # of Filters / sec
    // This counter displays the number of.NET exception filters executed per second.An exception filter evaluates whether an exception should be handled or not.
    // This counter tracks the rate of exception filters evaluated; irrespective of whether the exception was handled or not.
    // This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.
    // TODO: We need to count the delta.

    // # of Finallys / sec
    // This counter displays the number of finally blocks executed per second.
    // A finally block is guaranteed to be executed regardless of how the try block was exited.
    // Only the finally blocks that are executed for an exception are counted; finally blocks on normal code paths are not counted by this counter.
    // This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.
    // TODO: We need to count the delta.

    // Throw To Catch Depth / sec
    // This counter displays the number of stack frames traversed from the frame that threw the .NET exception to the frame that handled the exception per second.
    // This counter resets to 0 when an exception handler is entered; so nested exceptions would show the handler to handler stack depth.
    // This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.
    // TODO: We need to count the delta.

    // This counter displays the current number of Com-Callable-Wrappers (CCWs).
    // A CCW is a proxy for the .NET managed object being referenced from unmanaged COM client(s).
    // This counter was designed to indicate the number of managed objects being referenced by unmanaged COM code.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_INTEROP, DOTNET_INDEX_INTEROP_CCWCOUNT, L"# of CCWs", NULL);

    // This counter displays the current number of stubs created by the CLR.
    // Stubs are responsible for marshalling arguments and return values from managed to unmanaged code and vice versa; during a COM Interop call or PInvoke call.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_INTEROP, DOTNET_INDEX_INTEROP_STUBCOUNT, L"# of Stubs", NULL);

    // This counter displays the total number of times arguments and return values have been marshaled from managed to unmanaged code
    // and vice versa since the start of the application. This counter is not incremented if the stubs are inlined.
    // (Stubs are responsible for marshalling arguments and return values). Stubs usually get inlined if the marshalling overhead is small.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_INTEROP, DOTNET_INDEX_INTEROP_MARSHALCOUNT, L"# of Marshalling", NULL);

    // Reserved for future use.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_INTEROP, DOTNET_INDEX_INTEROP_TLBIMPORTPERSEC, L"# of TLB imports / sec", NULL);

    // Reserved for future use.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_INTEROP, DOTNET_INDEX_INTEROP_TLBEXPORTPERSEC, L"# of TLB exports / sec", NULL);

    // This counter displays the total number of methods compiled Just-In-Time (JIT) by the CLR JIT compiler since the start of the application.
    // This counter does not include the pre-jitted methods.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_JIT, DOTNET_INDEX_JIT_ILMETHODSJITTED, L"# of Methods Jitted", NULL);

    // This counter displays the total IL bytes jitted since the start of the application.
    // This counter is exactly equivalent to the "Total # of IL Bytes Jitted" counter.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_JIT, DOTNET_INDEX_JIT_ILBYTESJITTED, L"# of IL Bytes Jitted", NULL);

    // This counter displays the total IL bytes jitted since the start of the application.
    // This counter is exactly equivalent to the "# of IL Bytes Jitted" counter.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_JIT, DOTNET_INDEX_JIT_ILTOTALBYTESJITTED, L"Total # of IL Bytes Jitted", NULL);

    // This counter displays the peak number of methods the JIT compiler has failed to JIT since the start of the application.
    // This failure can occur if the IL cannot be verified or if there was an internal error in the JIT compiler.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_JIT, DOTNET_INDEX_JIT_FAILURES, L"Jit Failures", NULL);

    // This counter displays the percentage of elapsed time spent in JIT compilation since the last JIT compilation phase.
    // This counter is updated at the end of every JIT compilation phase. A JIT compilation phase is the phase when a method and its dependencies are being compiled.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_JIT, DOTNET_INDEX_JIT_TIME, L"% Time in Jit", NULL);

    // IL Bytes Jitted / sec
    // This counter displays the rate at which IL bytes are jitted per second.
    // This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.
    // TODO: We need to count the delta.

    // This counter displays the current number of classes loaded in all Assemblies.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_LOADING, DOTNET_INDEX_LOADING_CURRENTLOADED, L"Current Classes Loaded", NULL);

    // This counter displays the cumulative number of classes loaded in all Assemblies since the start of this application.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_LOADING, DOTNET_INDEX_LOADING_TOTALLOADED, L"Total Classes Loaded", NULL);

    // This counter displays the current number of AppDomains loaded in this application.
    // AppDomains (application domains) provide a secure and versatile unit of processing that the CLR can use to provide isolation between applications running in the same process.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_LOADING, DOTNET_INDEX_LOADING_CURRENTAPPDOMAINS, L"Current Appdomains", NULL);

    // This counter displays the peak number of AppDomains loaded since the start of this application.
    // AppDomains (application domains) provide a secure and versatile unit of processing that the CLR can use to provide isolation between applications running in the same process.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_LOADING, DOTNET_INDEX_LOADING_TOTALAPPDOMAINS, L"Total Appdomains", NULL);

    // This counter displays the current number of Assemblies loaded across all AppDomains in this application.
    // If the Assembly is loaded as domain - neutral from multiple AppDomains then this counter is incremented once only.
    // Assemblies can be loaded as domain - neutral when their code can be shared by all AppDomains or they can be loaded as domain - specific when their code is private to the AppDomain.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_LOADING, DOTNET_INDEX_LOADING_CURRENTASSEMBLIES, L"Current Assemblies", NULL);

    // This counter displays the total number of Assemblies loaded since the start of this application.
    // If the Assembly is loaded as domain - neutral from multiple AppDomains then this counter is incremented once only.
    // Assemblies can be loaded as domain - neutral when their code can be shared by all AppDomains or they can be loaded as domain - specific when their code is private to the AppDomain.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_LOADING, DOTNET_INDEX_LOADING_TOTALASSEMBLIES, L"Total Assemblies", NULL);

    // Reserved for future use.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_LOADING, DOTNET_INDEX_LOADING_ASSEMBLYSEARCHLENGTH, L"Assembly Search Length", NULL);

    // This counter displays the peak number of classes that have failed to load since the start of the application.
    // These load failures could be due to many reasons like inadequate security or illegal format.Full details can be found in the profiling services help.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_LOADING, DOTNET_INDEX_LOADING_TOTALLOADFAILURES, L"Total # of Load Failures", NULL);

    // This counter displays the current size(in bytes) of the memory committed by the class loader across all AppDomains.
    // (Committed memory is the physical memory for which space has been reserved on the disk paging file.)
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_LOADING, DOTNET_INDEX_LOADING_BYTESINLOADERHEAP, L"Bytes in Loader Heap", NULL);

    // This counter displays the total number of AppDomains unloaded since the start of the application.
    //If an AppDomain is loaded and unloaded multiple times this counter would count each of those unloads as separate.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_LOADING, DOTNET_INDEX_LOADING_TOTALAPPDOMAINSUNLOADED, L"Total Appdomains Unloaded", NULL);

    // Reserved for future use.
    //PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_LOADING, , L"% Time Loading", NULL);

    // Rate of Load Failures
    // This counter displays the number of classes that failed to load per second.
    // This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.
    // These load failures could be due to many reasons like inadequate security or illegal format. Full details can be found in the profiling services help.
    // TODO: We need to count the delta.

    // Rate of appdomains unloaded
    // This counter displays the number of AppDomains unloaded per second.
    // This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.
    // TODO: We need to count the delta.

    // Rate of Classes Loaded
    // This counter displays the number of classes loaded per second in all Assemblies.
    // This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.
    // TODO: We need to count the delta.

    // Rate of appdomains
    // This counter displays the number of AppDomains loaded per second.
    // AppDomains(application domains) provide a secure and versatile unit of processing that the CLR can use to provide isolation between applications
    // running in the same process. This counter is not an average over time; it displays the difference between the values observed in the last two samples
    // divided by the duration of the sample interval.
    // TODO: We need to count the delta.

    // Rate of Assemblies
    // This counter displays the number of Assemblies loaded across all AppDomains per second.
    // If the Assembly is loaded as domain - neutral from multiple AppDomains then this counter is incremented once only.Assemblies can be loaded as
    // domain - neutral when their code can be shared by all AppDomains or they can be loaded as domain - specific when their code is private to the AppDomain.
    // This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.
    // TODO: We need to count the delta.

    // This counter displays the total number of times threads in the CLR have attempted to acquire a managed lock unsuccessfully.
    // Managed locks can be acquired in many ways; by the "lock" statement in C# or by calling System.Monitor.Enter or by using MethodImplOptions.Synchronized custom attribute.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_LOCKSANDTHREADS, DOTNET_INDEX_LOCKSANDTHREADS_TOTALLOCKS, L"Total # of Contentions", NULL);

    // This counter displays the total number of threads currently waiting to acquire some managed lock in the application.
    // This counter is not an average over time; it displays the last observed value.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_LOCKSANDTHREADS, DOTNET_INDEX_LOCKSANDTHREADS_TOTALQUEUELENGTH, L"Current Queue Length", NULL);

    // This counter displays the total number of threads that waited to acquire some managed lock since the start of the application.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_LOCKSANDTHREADS, DOTNET_INDEX_LOCKSANDTHREADS_QUEUELENGTHPEAK, L"Queue Length Peak", NULL);

    // This counter displays the number of current.NET thread objects in the application.
    // A.NET thread object is created either by new System.Threading.Thread or when an unmanaged thread enters the managed environment.
    // This counters maintains the count of both running and stopped threads. This counter is not an average over time; it just displays the last observed value.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_LOCKSANDTHREADS, DOTNET_INDEX_LOCKSANDTHREADS_CURRENTLOGICAL, L"# of Current Logical Threads", NULL);

    // This counter displays the number of native OS threads created and owned by the CLR to act as underlying threads for .NET thread objects.
    // This counters value does not include the threads used by the CLR in its internal operations; it is a subset of the threads in the OS process.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_LOCKSANDTHREADS, DOTNET_INDEX_LOCKSANDTHREADS_CURRENTPHYSICAL, L"# of Current Physical Threads", NULL);

    // This counter displays the number of threads that are currently recognized by the CLR; they have a corresponding .NET thread object associated with them.
    // These threads are not created by the CLR; they are created outside the CLR but have since run inside the CLR at least once.
    // Only unique threads are tracked; threads with same thread ID re-entering the CLR or recreated after thread exit are not counted twice.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_LOCKSANDTHREADS, DOTNET_INDEX_LOCKSANDTHREADS_CURRENTRECOGNIZED, L"# of Current Recognized Threads", NULL);

    // This counter displays the total number of threads that have been recognized by the CLR since the start of this application;
    // these threads have a corresponding .NET thread object associated with them.
    // These threads are not created by the CLR; they are created outside the CLR but have since run inside the CLR at least once.
    // Only unique threads are tracked; threads with same thread ID re-entering the CLR or recreated after thread exit are not counted twice.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_LOCKSANDTHREADS, DOTNET_INDEX_LOCKSANDTHREADS_TOTALRECOGNIZED, L"# of Total Recognized Threads", NULL);

    // Contention Rate / sec
    // Rate at which threads in the runtime attempt to acquire a managed lock unsuccessfully.
    // Managed locks can be acquired in many ways; by the "lock" statement in C# or by calling System.Monitor.Enter or by using MethodImplOptions.Synchronized custom attribute.
    // TODO: We need to count the delta.

    // Queue Length / sec
    // This counter displays the number of threads per second waiting to acquire some lock in the application.
    // This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.
    // TODO: We need to count the delta.

    // rate of recognized threads / sec
    // This counter displays the number of threads per second that have been recognized by the CLR; these threads have a corresponding .NET thread object associated with them.
    // These threads are not created by the CLR; they are created outside the CLR but have since run inside the CLR at least once.
    // Only unique threads are tracked; threads with same thread ID re-entering the CLR or recreated after thread exit are not counted twice.
    // This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.
    // TODO: We need to count the delta.

    // This counter displays the number of times the generation 0 objects (youngest; most recently allocated) are garbage collected (Gen 0 GC) since the start of the application.
    // Gen 0 GC occurs when the available memory in generation 0 is not sufficient to satisfy an allocation request.
    // This counter is incremented at the end of a Gen 0 GC. Higher generation GCs include all lower generation GCs.
    // This counter is explicitly incremented when a higher generation (Gen 1 or Gen 2) GC occurs. _Global_ counter value is not accurate and should be ignored.
    // This counter displays the last observed value.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_MEMORY, DOTNET_INDEX_MEMORY_GENZEROCOLLECTIONS, L"# Gen 0 Collections", NULL);

    // This counter displays the number of times the generation 1 objects are garbage collected since the start of the application.
    // The counter is incremented at the end of a Gen 1 GC. Higher generation GCs include all lower generation GCs.
    // This counter is explicitly incremented when a higher generation (Gen 2) GC occurs. _Global_ counter value is not accurate and should be ignored.
    // This counter displays the last observed value.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_MEMORY, DOTNET_INDEX_MEMORY_GENONECOLLECTIONS, L"# Gen 1 Collections", NULL);

    // This counter displays the number of times the generation 2 objects(older) are garbage collected since the start of the application.
    // The counter is incremented at the end of a Gen 2 GC(also called full GC)._Global_ counter value is not accurate and should be ignored.
    // This counter displays the last observed value.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_MEMORY, DOTNET_INDEX_MEMORY_GENTWOCOLLECTIONS, L"# Gen 2 Collections", NULL);

    // This counter displays the bytes of memory that survive garbage collection(GC) and are promoted from generation 0 to generation 1;
    // objects that are promoted just because they are waiting to be finalized are not included in this counter.
    // This counter displays the value observed at the end of the last GC; its not a cumulative counter.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_MEMORY, DOTNET_INDEX_MEMORY_PROMOTEDFROMGENZERO, L"Promoted Memory from Gen 0", NULL);

    // This counter displays the bytes of memory that survive garbage collection(GC) and are promoted from generation 1 to generation 2;
    // objects that are promoted just because they are waiting to be finalized are not included in this counter.
    // This counter displays the value observed at the end of the last GC; its not a cumulative counter.
    // This counter is reset to 0 if the last GC was a Gen 0 GC only.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_MEMORY, DOTNET_INDEX_MEMORY_PROMOTEDFROMGENONE, L"Promoted Memory from Gen 1", NULL);

    // This counter displays the bytes of memory that are promoted from generation 0 to generation 1 just because they are waiting to be finalized.
    // This counter displays the value observed at the end of the last GC; its not a cumulative counter.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_MEMORY, DOTNET_INDEX_MEMORY_PROMOTEDFINALFROMGENZERO, L"Promoted Finalization-Memory from Gen 0", NULL);

    // Reserved for future use.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_MEMORY, DOTNET_INDEX_MEMORY_PROCESSID, L"Process ID", NULL);

    // This counter displays the maximum bytes that can be allocated in generation 0 (Gen 0);
    // its does not indicate the current number of bytes allocated in Gen 0.
    // A Gen 0 GC is triggered when the allocations since the last GC exceed this size.
    // The Gen 0 size is tuned by the Garbage Collector and can change during the execution of the application.
    // At the end of a Gen 0 collection the size of the Gen 0 heap is infact 0 bytes; this counter displays the size(in bytes) of allocations that would trigger
    // the next Gen 0 GC.This counter is updated at the end of a GC; its not updated on every allocation.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_MEMORY, DOTNET_INDEX_MEMORY_GENZEROHEAPSIZE, L"Gen 0 Heap Size", NULL);

    // This counter displays the current number of bytes in generation 1 (Gen 1);
    // this counter does not display the maximum size of Gen 1. Objects are not directly allocated in this generation;
    // they are promoted from previous Gen 0 GCs.This counter is updated at the end of a GC; its not updated on every allocation.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_MEMORY, DOTNET_INDEX_MEMORY_GENONEHEAPSIZE, L"Gen 1 Heap Size", NULL);

    // This counter displays the current number of bytes in generation 2 (Gen 2).
    // Objects are not directly allocated in this generation; they are promoted from Gen 1 during previous Gen 1 GCs.
    // This counter is updated at the end of a GC; its not updated on every allocation.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_MEMORY, DOTNET_INDEX_MEMORY_GENTWOHEAPSIZE, L"Gen 2 Heap Size", NULL);

    // This counter displays the current size of the Large Object Heap in bytes.
    // Objects greater than 20 KBytes are treated as large objects by the Garbage Collector and are directly allocated in a special heap; they are not promoted through the generations.
    // This counter is updated at the end of a GC; its not updated on every allocation.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_MEMORY, DOTNET_INDEX_MEMORY_LOHSIZE, L"Large Object Heap Size", NULL);

    // This counter displays the number of garbage collected objects that survive a collection because they are waiting to be finalized.
    // If these objects hold references to other objects then those objects also survive but are not counted by this counter; the "Promoted Finalization-Memory from Gen 0"
    // and "Promoted Finalization-Memory from Gen 1" counters represent all the memory that survived due to finalization.
    // This counter is not a cumulative counter; its updated at the end of every GC with count of the survivors during that particular GC only.
    // This counter was designed to indicate the extra overhead that the application might incur because of finalization.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_MEMORY, DOTNET_INDEX_MEMORY_FINALSURVIVORS, L"Finalization Survivors", NULL);

    // This counter displays the current number of GC Handles in-use.
    // GCHandles are handles to resources external to the CLR and the managed environment.
    // Handles occupy small amounts of memory in the GCHeap but potentially expensive unmanaged resources.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_MEMORY, DOTNET_INDEX_MEMORY_GCHANDLES, L"# GC Handles", NULL);

    // This counter displays the peak number of times a garbage collection was performed because of an explicit call to GC.Collect.
    // Its a good practice to let the GC tune the frequency of its collections.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_MEMORY, DOTNET_INDEX_MEMORY_INDUCEDGC, L"# Induced GC", NULL);

    // % Time in GC is the percentage of elapsed time that was spent in performing a garbage collection(GC) since the last GC cycle.
    // This counter is usually an indicator of the work done by the Garbage Collector on behalf of the application to collect and compact memory.
    // This counter is updated only at the end of every GC and the counter value reflects the last observed value; its not an average.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_MEMORY, DOTNET_INDEX_MEMORY_TIMEINGC, L"% Time in GC", NULL);

    // This counter is the sum of four other counters; Gen 0 Heap Size; Gen 1 Heap Size; Gen 2 Heap Size and the Large Object Heap Size.
    // This counter indicates the current memory allocated in bytes on the GC Heaps.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_MEMORY, DOTNET_INDEX_MEMORY_BYTESINALLHEAPS, L"# Bytes in all Heaps", NULL);

    // This counter displays the amount of virtual memory(in bytes) currently committed by the Garbage Collector.
    // (Committed memory is the physical memory for which space has been reserved on the disk paging file).
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_MEMORY, DOTNET_INDEX_MEMORY_TOTALCOMMITTED, L"# Total Committed Bytes", NULL);

    // This counter displays the amount of virtual memory(in bytes) currently reserved by the Garbage Collector.
    // (Reserved memory is the virtual memory space reserved for the application but no disk or main memory pages have been used.)
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_MEMORY, DOTNET_INDEX_MEMORY_TOTALRESERVED, L"# Total Reserved Bytes", NULL);

    // This counter displays the number of pinned objects encountered in the last GC.
    // This counter tracks the pinned objects only in the heaps that were garbage collected e.g. A Gen 0 GC would cause enumeration of pinned objects in the generation 0 heap only.
    // A pinned object is one that the Garbage Collector cannot move in memory.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_MEMORY, DOTNET_INDEX_MEMORY_TOTALPINNED, L"# of Pinned Objects", NULL);

    // This counter displays the current number of sync blocks in use. Sync blocks are per-object data structures allocated for storing synchronization information.
    // Sync blocks hold weak references to managed objects and need to be scanned by the Garbage Collector.
    // Sync blocks are not limited to storing synchronization information and can also store COM interop metadata.
    // This counter was designed to indicate performance problems with heavy use of synchronization primitives.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_MEMORY, DOTNET_INDEX_MEMORY_TOTALSINKS, L"# of Sink Blocks in use", NULL);

    // Reserved for future use.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_MEMORY, DOTNET_INDEX_MEMORY_TOTALBYTESSINCESTART, L"Total Bytes Allocated (since start)", NULL);

    // Reserved for future use.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_MEMORY, DOTNET_INDEX_MEMORY_TOTALLOHBYTESSINCESTART, L"Total Bytes Allocated for Large Objects (since start)", NULL);

    // Gen 0 Promoted Bytes / Sec
    // This counter displays the bytes per second that are promoted from generation 0 (youngest)to generation 1;
    // objects that are promoted just because they are waiting to be finalized are not included in this counter.
    // Memory is promoted when it survives a garbage collection.This counter was designed as an indicator of relatively long-lived objects being created per sec.
    // This counter displays the difference between the values observed in the last two samples divided by the duration of the sample interval.
    // TODO: We need to count the delta.

    // Gen 1 Promoted Bytes / Sec
    // This counter displays the bytes per second that are promoted from generation 1 to generation 2 (oldest);
    // objects that are promoted just because they are waiting to be finalized are not included in this counter.
    // Memory is promoted when it survives a garbage collection.
    // Nothing is promoted from generation 2 since it is the oldest.
    // This counter was designed as an indicator of very long-lived objects being created per sec.
    // This counter displays the difference between the values observed in the last two samples divided by the duration of the sample interval.
    // TODO: We need to count the delta.

    // Promoted Finalization - Memory from Gen 1
    // This counter displays the bytes of memory that are promoted from generation 1 to generation 2 just because they are waiting to be finalized.
    // This counter displays the value observed at the end of the last GC; its not a cumulative counter.This counter is reset to 0 if the last GC was a Gen 0 GC only.
    // TODO: We need to count the delta.

    // Allocated Bytes / sec
    // This counter displays the rate of bytes per second allocated on the GC Heap.
    // This counter is updated at the end of every GC; not at each allocation.
    // This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.
    // TODO: We need to count the delta.

    // This counter displays the total number of remote procedure calls invoked since the start of this application.
    // A remote procedure call is a call on any object outside the callers AppDomain.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_REMOTING, DOTNET_INDEX_REMOTING_TOTALREMOTECALLS, L"Total Remote Calls", NULL);

    // This counter displays the total number of remoting channels registered across all AppDomains since the start of the application.
    // Channels are used to transport messages to and from remote objects.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_REMOTING, DOTNET_INDEX_REMOTING_CHANNELS, L"Channels", NULL);

    // This counter displays the total number of remoting proxy objects created in this process since the start of the process.
    // Proxy object acts as a representative of the remote objects and ensures that all calls made on the proxy are forwarded to the correct remote object instance.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_REMOTING, DOTNET_INDEX_REMOTING_CONTEXTPROXIES, L"Context Proxies", NULL);

    // This counter displays the current number of context-bound classes loaded.
    // Classes that can be bound to a context are called context-bound classes; context-bound classes are marked with Context Attributes
    // which provide usage rules for synchronization; thread affinity; transactions etc.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_REMOTING, DOTNET_INDEX_REMOTING_CONTEXTCLASSESLOADED, L"Context-Bound Classes Loaded", NULL);

    // This counter displays the current number of remoting contexts in the application.
    // A context is a boundary containing a collection of objects with the same usage rules like synchronization; thread affinity; transactions etc.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_REMOTING, DOTNET_INDEX_REMOTING_CONTEXTS, L"Contexts", NULL);

    // Reserved for future use.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_REMOTING, DOTNET_INDEX_REMOTING_CONTEXTSALLOCATED, L"# of context bound objects allocated", NULL);

    // Remote Calls / sec
    // This counter displays the number of remote procedure calls invoked per second.
    // A remote procedure call is a call on any object outside the callers AppDomain.
    // This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.
    // TODO: We need to count the delta.

    // Context - Bound Objects Alloc / sec
    // This counter displays the number of context - bound objects allocated per second.
    // Instances of classes that can be bound to a context are called context - bound objects; context - bound classes are marked with Context Attributes
    // which provide usage rules for synchronization; thread affinity; transactions etc.
    // This counter is not an average over time; it displays the difference between the values observed in the last two samples divided by the duration of the sample interval.
    // TODO: We need to count the delta.

    // This counter displays the total number of runtime Code Access Security(CAS) checks performed since the start of the application.
    // Runtime CAS checks are performed when a caller makes a call to a callee demanding a particular permission;
    // the runtime check is made on every call by the caller; the check is done by examining the current thread stack of the caller.
    // This counter used together with "Stack Walk Depth" is indicative of performance penalty for security checks.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_SECURITY, DOTNET_INDEX_SECURITY_TOTALRUNTIMECHECKS, L"Total Runtime Checks", NULL);

    // This counter displays the total number of linktime Code Access Security(CAS) checks since the start of the application.
    // Linktime CAS checks are performed when a caller makes a call to a callee demanding a particular permission at JIT compile time; linktime check is performed once per caller.
    // This count is not indicative of serious performance issues; its indicative of the security system activity.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_SECURITY, DOTNET_INDEX_SECURITY_LINKTIMECHECKS, L"# Link Time Checks", NULL);

    // This counter displays the percentage of elapsed time spent in performing runtime Code Access Security(CAS) checks since the last such check.
    // CAS allows code to be trusted to varying degrees and enforces these varying levels of trust depending on code identity.
    // This counter is updated at the end of a runtime security check; it represents the last observed value; its not an average.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_SECURITY, DOTNET_INDEX_SECURITY_TIMEINRTCHECKS, L"% Time in RT checks", NULL);

    // This counter displays the depth of the stack during that last runtime Code Access Security check.
    // Runtime Code Access Security check is performed by crawling the stack.
    // This counter is not an average; it just displays the last observed value.
    PhAddListViewGroupItem(ListViewHandle, DOTNET_CATEGORY_SECURITY, DOTNET_INDEX_SECURITY_STACKWALKDEPTH, L"Stack Walk Depth", NULL);

    // % Time Sig.Authenticating
    // Reserved for future use.
}

VOID DotNetPerfAddProcessAppDomains(
    _In_ HWND hwndDlg,
    _In_ PPERFPAGE_CONTEXT Context
    )
{
    ExtendedListView_SetRedraw(Context->AppDomainsLv, FALSE);
    ListView_DeleteAllItems(Context->AppDomainsLv);

    if (Context->ClrV4)
    {
        PPH_LIST processAppDomains = QueryDotNetAppDomainsForPid_V4(
            Context->IsWow64,
            Context->ProcessHandle,
            Context->ProcessItem->ProcessId
            );

        if (processAppDomains)
        {
            for (ULONG i = 0; i < processAppDomains->Count; i++)
            {
                PhAddListViewItem(Context->AppDomainsLv, MAXINT, processAppDomains->Items[i], NULL);
                PhFree(processAppDomains->Items[i]);
            }

            PhDereferenceObject(processAppDomains);
        }
    }
    else
    {
        PPH_LIST processAppDomains = QueryDotNetAppDomainsForPid_V2(
            Context->IsWow64,
            Context->ProcessHandle,
            Context->ProcessItem->ProcessId
            );

        if (processAppDomains)
        {
            for (ULONG i = 0; i < processAppDomains->Count; i++)
            {
                PhAddListViewItem(Context->AppDomainsLv, MAXINT, processAppDomains->Items[i], NULL);
                PhFree(processAppDomains->Items[i]);
            }

            PhDereferenceObject(processAppDomains);
        }
    }

    ExtendedListView_SetRedraw(Context->AppDomainsLv, TRUE);
}

VOID DotNetPerfUpdateCounterData(
    _In_ HWND hwndDlg,
    _In_ PPERFPAGE_CONTEXT Context
    )
{
    PVOID perfStatBlock = NULL;
    Perf_GC dotNetPerfGC;
    Perf_Contexts dotNetPerfContext;
    Perf_Interop dotNetPerfInterop;
    Perf_Loading dotNetPerfLoading;
    Perf_Excep dotNetPerfExcep;
    Perf_LocksAndThreads dotNetPerfLocksAndThreads;
    Perf_Jit dotNetPerfJit;
    Perf_Security dotNetPerfSecurity;

    if (Context->ClrV4)
    {
        perfStatBlock = GetPerfIpcBlock_V4(
            Context->IsWow64,
            Context->BlockTableAddress
            );
    }
    else
    {
        perfStatBlock = GetPerfIpcBlock_V2(
            Context->IsWow64,
            Context->BlockTableAddress
            );
    }

    if (!perfStatBlock)
        return;

    if (Context->IsWow64)
    {
        PerfCounterIPCControlBlock_Wow64* perfBlock = perfStatBlock;
        Perf_GC_Wow64 dotNetPerfGC_Wow64 = perfBlock->GC;
        Perf_Loading_Wow64 dotNetPerfLoading_Wow64 = perfBlock->Loading;
        Perf_Security_Wow64 dotNetPerfSecurity_Wow64 = perfBlock->Security;

        // Thunk the Wow64 structures into their 64bit versions (or 32bit version on x86).

        dotNetPerfGC.cGenCollections[0] = dotNetPerfGC_Wow64.cGenCollections[0];
        dotNetPerfGC.cGenCollections[1] = dotNetPerfGC_Wow64.cGenCollections[1];
        dotNetPerfGC.cGenCollections[2] = dotNetPerfGC_Wow64.cGenCollections[2];
        dotNetPerfGC.cbPromotedMem[0] = dotNetPerfGC_Wow64.cbPromotedMem[0];
        dotNetPerfGC.cbPromotedMem[1] = dotNetPerfGC_Wow64.cbPromotedMem[1];
        dotNetPerfGC.cbPromotedFinalizationMem = dotNetPerfGC_Wow64.cbPromotedFinalizationMem;
        dotNetPerfGC.cProcessID = dotNetPerfGC_Wow64.cProcessID;
        dotNetPerfGC.cGenHeapSize[0] = dotNetPerfGC_Wow64.cGenHeapSize[0];
        dotNetPerfGC.cGenHeapSize[1] = dotNetPerfGC_Wow64.cGenHeapSize[1];
        dotNetPerfGC.cGenHeapSize[2] = dotNetPerfGC_Wow64.cGenHeapSize[2];
        dotNetPerfGC.cTotalCommittedBytes = dotNetPerfGC_Wow64.cTotalCommittedBytes;
        dotNetPerfGC.cTotalReservedBytes = dotNetPerfGC_Wow64.cTotalReservedBytes;
        dotNetPerfGC.cLrgObjSize = dotNetPerfGC_Wow64.cLrgObjSize;
        dotNetPerfGC.cSurviveFinalize = dotNetPerfGC_Wow64.cSurviveFinalize;
        dotNetPerfGC.cHandles = dotNetPerfGC_Wow64.cHandles;
        dotNetPerfGC.cbAlloc = dotNetPerfGC_Wow64.cbAlloc;
        dotNetPerfGC.cbLargeAlloc = dotNetPerfGC_Wow64.cbLargeAlloc;
        dotNetPerfGC.cInducedGCs = dotNetPerfGC_Wow64.cInducedGCs;
        dotNetPerfGC.timeInGC = dotNetPerfGC_Wow64.timeInGC;
        dotNetPerfGC.timeInGCBase = dotNetPerfGC_Wow64.timeInGCBase;
        dotNetPerfGC.cPinnedObj = dotNetPerfGC_Wow64.cPinnedObj;
        dotNetPerfGC.cSinkBlocks = dotNetPerfGC_Wow64.cSinkBlocks;

        dotNetPerfContext = perfBlock->Context;
        dotNetPerfInterop = perfBlock->Interop;

        dotNetPerfLoading.cClassesLoaded.Current = dotNetPerfLoading_Wow64.cClassesLoaded.Current;
        dotNetPerfLoading.cClassesLoaded.Total = dotNetPerfLoading_Wow64.cClassesLoaded.Total;
        dotNetPerfLoading.cAppDomains.Current = dotNetPerfLoading_Wow64.cAppDomains.Current;
        dotNetPerfLoading.cAppDomains.Total = dotNetPerfLoading_Wow64.cAppDomains.Total;
        dotNetPerfLoading.cAssemblies.Current = dotNetPerfLoading_Wow64.cAssemblies.Current;
        dotNetPerfLoading.cAssemblies.Total = dotNetPerfLoading_Wow64.cAssemblies.Total;
        dotNetPerfLoading.timeLoading = dotNetPerfLoading_Wow64.timeLoading;
        dotNetPerfLoading.cAsmSearchLen = dotNetPerfLoading_Wow64.cAsmSearchLen;
        dotNetPerfLoading.cLoadFailures.Total = dotNetPerfLoading_Wow64.cLoadFailures.Total;
        dotNetPerfLoading.cbLoaderHeapSize = dotNetPerfLoading_Wow64.cbLoaderHeapSize;
        dotNetPerfLoading.cAppDomainsUnloaded = dotNetPerfLoading_Wow64.cAppDomainsUnloaded;

        dotNetPerfExcep = perfBlock->Excep;
        dotNetPerfLocksAndThreads = perfBlock->LocksAndThreads;
        dotNetPerfJit = perfBlock->Jit;

        dotNetPerfSecurity.cTotalRTChecks = dotNetPerfSecurity_Wow64.cTotalRTChecks;
        dotNetPerfSecurity.timeAuthorize = dotNetPerfSecurity_Wow64.timeAuthorize;
        dotNetPerfSecurity.cLinkChecks = dotNetPerfSecurity_Wow64.cLinkChecks;
        dotNetPerfSecurity.timeRTchecks = dotNetPerfSecurity_Wow64.timeRTchecks;
        dotNetPerfSecurity.timeRTchecksBase = dotNetPerfSecurity_Wow64.timeRTchecksBase;
        dotNetPerfSecurity.stackWalkDepth = dotNetPerfSecurity_Wow64.stackWalkDepth;
    }
    else
    {
        PerfCounterIPCControlBlock* perfBlock = perfStatBlock;

        dotNetPerfGC = perfBlock->GC;
        dotNetPerfContext = perfBlock->Context;
        dotNetPerfInterop = perfBlock->Interop;
        dotNetPerfLoading = perfBlock->Loading;
        dotNetPerfExcep = perfBlock->Excep;
        dotNetPerfLocksAndThreads = perfBlock->LocksAndThreads;
        dotNetPerfJit = perfBlock->Jit;
        dotNetPerfSecurity = perfBlock->Security;
    }

    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_EXCEPTIONS_THROWNCOUNT, 1, PhaFormatUInt64(dotNetPerfExcep.cThrown.Total, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_EXCEPTIONS_FILTERSCOUNT, 1, PhaFormatUInt64(dotNetPerfExcep.cFiltersExecuted, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_EXCEPTIONS_FINALLYCOUNT, 1, PhaFormatUInt64(dotNetPerfExcep.cFinallysExecuted, TRUE)->Buffer);
    //PhSetListViewSubItem(Context->CountersLv, , 1, PhaFormatUInt64(dotNetPerfExcep.cThrowToCatchStackDepth, TRUE)->Buffer);

    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_INTEROP_CCWCOUNT, 1, PhaFormatUInt64(dotNetPerfInterop.cCCW, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_INTEROP_STUBCOUNT, 1, PhaFormatUInt64(dotNetPerfInterop.cStubs, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_INTEROP_MARSHALCOUNT, 1, PhaFormatUInt64(dotNetPerfInterop.cMarshalling, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_INTEROP_TLBIMPORTPERSEC, 1, PhaFormatUInt64(dotNetPerfInterop.cTLBImports, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_INTEROP_TLBEXPORTPERSEC, 1, PhaFormatUInt64(dotNetPerfInterop.cTLBExports, TRUE)->Buffer);

    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_JIT_ILMETHODSJITTED, 1, PhaFormatUInt64(dotNetPerfJit.cMethodsJitted, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_JIT_ILBYTESJITTED, 1, PhaFormatSize(dotNetPerfJit.cbILJitted.Current, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_JIT_ILTOTALBYTESJITTED, 1, PhaFormatSize(dotNetPerfJit.cbILJitted.Total, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_JIT_FAILURES, 1, PhaFormatUInt64(dotNetPerfJit.cJitFailures, TRUE)->Buffer);

    if (dotNetPerfJit.timeInJitBase != 0)
    {
        PH_FORMAT format[1];
        WCHAR formatBuffer[10];

        // TODO: TimeInJit is always above 100% for some processes ??
        // SeeAlso: https://github.com/dotnet/coreclr/blob/master/src/gc/gcee.cpp#L324
        PhInitFormatF(&format[0], (dotNetPerfJit.timeInJit << 8) * 100 / (FLOAT)(dotNetPerfJit.timeInJitBase << 8), 2);

        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
            PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_JIT_TIME, 1, formatBuffer);
    }
    else
    {
        PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_JIT_TIME, 1, L"0.00");
    }

    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_LOADING_CURRENTLOADED, 1, PhaFormatUInt64(dotNetPerfLoading.cClassesLoaded.Current, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_LOADING_TOTALLOADED, 1, PhaFormatUInt64(dotNetPerfLoading.cClassesLoaded.Total, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_LOADING_CURRENTAPPDOMAINS, 1, PhaFormatUInt64(dotNetPerfLoading.cAppDomains.Current, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_LOADING_TOTALAPPDOMAINS, 1, PhaFormatUInt64(dotNetPerfLoading.cAppDomains.Total, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_LOADING_CURRENTASSEMBLIES, 1, PhaFormatUInt64(dotNetPerfLoading.cAssemblies.Current, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_LOADING_TOTALASSEMBLIES, 1, PhaFormatUInt64(dotNetPerfLoading.cAssemblies.Total, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_LOADING_ASSEMBLYSEARCHLENGTH, 1, PhaFormatUInt64(dotNetPerfLoading.cAsmSearchLen, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_LOADING_TOTALLOADFAILURES, 1, PhaFormatUInt64(dotNetPerfLoading.cLoadFailures.Total, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_LOADING_BYTESINLOADERHEAP, 1, PhaFormatSize(dotNetPerfLoading.cbLoaderHeapSize, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_LOADING_TOTALAPPDOMAINSUNLOADED, 1, PhaFormatUInt64(dotNetPerfLoading.cAppDomainsUnloaded.Total, TRUE)->Buffer);
    //PhSetListViewSubItem(Context->CountersLv, 10, 1, PhaFormatUInt64(dotNetPerfLoading.timeLoading, TRUE)->Buffer);

    // DOTNET_CATEGORY_LOCKSANDTHREADS
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_LOCKSANDTHREADS_TOTALLOCKS, 1, PhaFormatUInt64(dotNetPerfLocksAndThreads.cContention.Total, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_LOCKSANDTHREADS_TOTALQUEUELENGTH, 1, PhaFormatUInt64(dotNetPerfLocksAndThreads.cQueueLength.Current, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_LOCKSANDTHREADS_QUEUELENGTHPEAK, 1, PhaFormatUInt64(dotNetPerfLocksAndThreads.cQueueLength.Total, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_LOCKSANDTHREADS_CURRENTLOGICAL, 1, PhaFormatUInt64(dotNetPerfLocksAndThreads.cCurrentThreadsLogical, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_LOCKSANDTHREADS_CURRENTPHYSICAL, 1, PhaFormatUInt64(dotNetPerfLocksAndThreads.cCurrentThreadsPhysical, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_LOCKSANDTHREADS_CURRENTRECOGNIZED, 1, PhaFormatUInt64(dotNetPerfLocksAndThreads.cRecognizedThreads.Current, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_LOCKSANDTHREADS_TOTALRECOGNIZED, 1, PhaFormatUInt64(dotNetPerfLocksAndThreads.cRecognizedThreads.Total, TRUE)->Buffer);

    // DOTNET_CATEGORY_MEMORY
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_MEMORY_GENZEROCOLLECTIONS, 1, PhaFormatUInt64(dotNetPerfGC.cGenCollections[0], TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_MEMORY_GENONECOLLECTIONS, 1, PhaFormatUInt64(dotNetPerfGC.cGenCollections[1], TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_MEMORY_GENTWOCOLLECTIONS, 1, PhaFormatUInt64(dotNetPerfGC.cGenCollections[2], TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_MEMORY_PROMOTEDFROMGENZERO, 1, PhaFormatSize(dotNetPerfGC.cbPromotedMem[0], ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_MEMORY_PROMOTEDFROMGENONE, 1, PhaFormatSize(dotNetPerfGC.cbPromotedMem[1], ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_MEMORY_PROMOTEDFINALFROMGENZERO, 1, PhaFormatSize(dotNetPerfGC.cbPromotedFinalizationMem, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_MEMORY_PROCESSID, 1, PhaFormatUInt64(dotNetPerfGC.cProcessID, FALSE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_MEMORY_GENZEROHEAPSIZE, 1, PhaFormatSize(dotNetPerfGC.cGenHeapSize[0], ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_MEMORY_GENONEHEAPSIZE, 1, PhaFormatSize(dotNetPerfGC.cGenHeapSize[1], ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_MEMORY_GENTWOHEAPSIZE, 1, PhaFormatSize(dotNetPerfGC.cGenHeapSize[2], ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_MEMORY_LOHSIZE, 1, PhaFormatSize(dotNetPerfGC.cLrgObjSize, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_MEMORY_FINALSURVIVORS, 1, PhaFormatUInt64(dotNetPerfGC.cSurviveFinalize, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_MEMORY_GCHANDLES, 1, PhaFormatUInt64(dotNetPerfGC.cHandles, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_MEMORY_INDUCEDGC, 1, PhaFormatUInt64(dotNetPerfGC.cInducedGCs, TRUE)->Buffer);

    if (dotNetPerfGC.timeInGCBase != 0)
    {
        PH_FORMAT format[1];
        WCHAR formatBuffer[10];

        PhInitFormatF(&format[0], (FLOAT)dotNetPerfGC.timeInGC * 100 / (FLOAT)dotNetPerfGC.timeInGCBase, 2);

        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
            PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_MEMORY_TIMEINGC, 1, formatBuffer);
    }
    else
    {
        PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_MEMORY_TIMEINGC, 1, L"0.00");
    }

    //  The CLR source says this value should be "Gen 0 Heap Size; Gen 1 Heap Size; Gen 2 Heap Size and the Large Object Heap Size",
    //    but Perflib only counts Gen 1, Gen 2 and cLrgObjSize... For now, just do what perflib does.
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_MEMORY_BYTESINALLHEAPS, 1, PhaFormatSize(dotNetPerfGC.cGenHeapSize[1] + dotNetPerfGC.cGenHeapSize[2] + dotNetPerfGC.cLrgObjSize, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_MEMORY_TOTALCOMMITTED, 1, PhaFormatSize(dotNetPerfGC.cTotalCommittedBytes, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_MEMORY_TOTALRESERVED, 1, PhaFormatSize(dotNetPerfGC.cTotalReservedBytes, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_MEMORY_TOTALPINNED, 1, PhaFormatUInt64(dotNetPerfGC.cPinnedObj, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_MEMORY_TOTALSINKS, 1, PhaFormatUInt64(dotNetPerfGC.cSinkBlocks, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_MEMORY_TOTALBYTESSINCESTART, 1, PhaFormatSize(dotNetPerfGC.cbAlloc, ULONG_MAX)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_MEMORY_TOTALLOHBYTESSINCESTART, 1, PhaFormatSize(dotNetPerfGC.cbLargeAlloc, ULONG_MAX)->Buffer);

    // DOTNET_CATEGORY_REMOTING
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_REMOTING_TOTALREMOTECALLS, 1, PhaFormatUInt64(dotNetPerfContext.cRemoteCalls.Total, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_REMOTING_CHANNELS, 1, PhaFormatUInt64(dotNetPerfContext.cChannels, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_REMOTING_CONTEXTPROXIES, 1, PhaFormatUInt64(dotNetPerfContext.cProxies, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_REMOTING_CONTEXTCLASSESLOADED, 1, PhaFormatUInt64(dotNetPerfContext.cClasses, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_REMOTING_CONTEXTS, 1, PhaFormatUInt64(dotNetPerfContext.cContexts, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_REMOTING_CONTEXTSALLOCATED, 1, PhaFormatUInt64(dotNetPerfContext.cObjAlloc, TRUE)->Buffer);

    // DOTNET_CATEGORY_SECURITY
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_SECURITY_TOTALRUNTIMECHECKS, 1, PhaFormatUInt64(dotNetPerfSecurity.cTotalRTChecks, TRUE)->Buffer);
    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_SECURITY_LINKTIMECHECKS, 1, PhaFormatUInt64(dotNetPerfSecurity.cLinkChecks, TRUE)->Buffer);

    if (dotNetPerfSecurity.timeRTchecksBase != 0)
    {
        PH_FORMAT format[1];
        WCHAR formatBuffer[10];

        PhInitFormatF(&format[0], (FLOAT)dotNetPerfSecurity.timeRTchecks * 100 / (FLOAT)dotNetPerfSecurity.timeRTchecksBase, 2);

        if (PhFormatToBuffer(format, RTL_NUMBER_OF(format), formatBuffer, sizeof(formatBuffer), NULL))
            PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_SECURITY_TIMEINRTCHECKS, 1, formatBuffer);
    }
    else
    {
        PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_SECURITY_TIMEINRTCHECKS, 1, L"0.00");
    }

    PhSetListViewSubItem(Context->CountersLv, DOTNET_INDEX_SECURITY_STACKWALKDEPTH, 1, PhaFormatUInt64(dotNetPerfSecurity.stackWalkDepth, TRUE)->Buffer);
}

INT_PTR CALLBACK DotNetPerfPageDlgProc(
    _In_ HWND hwndDlg,
    _In_ UINT uMsg,
    _In_ WPARAM wParam,
    _In_ LPARAM lParam
    )
{
    LPPROPSHEETPAGE propSheetPage;
    PPH_PROCESS_PROPPAGECONTEXT propPageContext;
    PPH_PROCESS_ITEM processItem;
    PPERFPAGE_CONTEXT context;

    if (PhPropPageDlgProcHeader(hwndDlg, uMsg, lParam, &propSheetPage, &propPageContext, &processItem))
    {
        context = propPageContext->Context;
    }
    else
    {
        return FALSE;
    }

    switch (uMsg)
    {
    case WM_INITDIALOG:
        {
            context = propPageContext->Context = PhAllocateZero(sizeof(PERFPAGE_CONTEXT));
            context->WindowHandle = hwndDlg;
            context->ProcessItem = processItem;
            context->AppDomainsLv = GetDlgItem(hwndDlg, IDC_APPDOMAINS);
            context->CountersLv = GetDlgItem(hwndDlg, IDC_COUNTERS);
            context->Enabled = TRUE;

            PhSetListViewStyle(context->AppDomainsLv, FALSE, TRUE);
            PhSetControlTheme(context->AppDomainsLv, L"explorer");
            PhAddListViewColumn(context->AppDomainsLv, 0, 0, 0, LVCFMT_LEFT, 300, L"Application domain");
            PhSetExtendedListView(context->AppDomainsLv);

            PhSetListViewStyle(context->CountersLv, FALSE, TRUE);
            PhSetControlTheme(context->CountersLv, L"explorer");
            PhAddListViewColumn(context->CountersLv, 0, 0, 0, LVCFMT_LEFT, 250, L"Counter");
            PhAddListViewColumn(context->CountersLv, 1, 1, 1, LVCFMT_RIGHT, 140, L"Value");
            PhSetExtendedListView(context->CountersLv);

            DotNetPerfAddListViewGroups(context->CountersLv);
            PhLoadListViewColumnsFromSetting(SETTING_NAME_DOT_NET_COUNTERS_COLUMNS, context->CountersLv);

#ifdef _WIN64
            context->IsWow64 = !!context->ProcessItem->IsWow64;
#else
            // HACK: Work-around for Appdomain enumeration on 32bit.
            context->IsWow64 = TRUE;
#endif

            if (NT_SUCCESS(PhOpenProcess(
                &context->ProcessHandle,
                PROCESS_VM_READ | ProcessQueryAccess | PROCESS_DUP_HANDLE | SYNCHRONIZE,
                context->ProcessItem->ProcessId
                )))
            {
                ULONG flags = 0;

                if (NT_SUCCESS(PhGetProcessIsDotNetEx(
                    context->ProcessItem->ProcessId,
                    context->ProcessHandle,
                    context->ProcessItem->IsImmersive == 1 ? 0 : PH_CLR_USE_SECTION_CHECK,
                    NULL,
                    &flags
                    )))
                {
                    if (flags & PH_CLR_VERSION_4_ABOVE)
                    {
                        context->ClrV4 = TRUE;
                    }
                }

                // Skip AppDomain enumeration of 'Modern' .NET applications as they don't expose the CLR 'Private IPC' block.
                if (!context->ProcessItem->IsImmersive)
                {
                    DotNetPerfAddProcessAppDomains(hwndDlg, context);
                }
            }

            if (context->ClrV4)
            {
                if (OpenDotNetPublicControlBlock_V4(
                    !!context->ProcessItem->IsImmersive,
                    context->ProcessHandle,
                    context->ProcessItem->ProcessId,
                    &context->BlockTableHandle,
                    &context->BlockTableAddress
                    ))
                {
                    context->ControlBlockValid = TRUE;
                }
            }
            else
            {
                if (OpenDotNetPublicControlBlock_V2(
                    context->ProcessItem->ProcessId,
                    &context->BlockTableHandle,
                    &context->BlockTableAddress
                    ))
                {
                    context->ControlBlockValid = TRUE;
                }
            }

            if (context->ControlBlockValid)
            {
                DotNetPerfUpdateCounterData(hwndDlg, context);
            }

            PhRegisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessesUpdated),
                DotNetPerfProcessesUpdatedCallback,
                context,
                &context->ProcessesUpdatedCallbackRegistration
                );

            PhInitializeWindowTheme(hwndDlg, !!PhGetIntegerSetting(L"EnableThemeSupport"));
        }
        break;
    case WM_DESTROY:
        {
            PhUnregisterCallback(
                PhGetGeneralCallback(GeneralCallbackProcessesUpdated),
                &context->ProcessesUpdatedCallbackRegistration
                );

            if (context->BlockTableAddress)
            {
                NtUnmapViewOfSection(NtCurrentProcess(), context->BlockTableAddress);
            }

            if (context->BlockTableHandle)
            {
                NtClose(context->BlockTableHandle);
            }

            if (context->ProcessHandle)
            {
                NtClose(context->ProcessHandle);
            }

            PhSaveListViewColumnsToSetting(SETTING_NAME_DOT_NET_COUNTERS_COLUMNS, context->CountersLv);

            PhFree(context);
        }
        break;
    case WM_SHOWWINDOW:
        {
            PPH_LAYOUT_ITEM dialogItem;

            if (dialogItem = PhBeginPropPageLayout(hwndDlg, propPageContext))
            {
                PhAddPropPageLayoutItem(hwndDlg, context->AppDomainsLv, dialogItem, PH_ANCHOR_LEFT | PH_ANCHOR_TOP | PH_ANCHOR_RIGHT);
                PhAddPropPageLayoutItem(hwndDlg, context->CountersLv, dialogItem, PH_ANCHOR_ALL);
                PhEndPropPageLayout(hwndDlg, propPageContext);
            }

            ExtendedListView_SetColumnWidth(context->AppDomainsLv, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
        }
        break;
    case WM_NOTIFY:
        {
            LPNMHDR header = (LPNMHDR)lParam;

            switch (header->code)
            {
            case PSN_SETACTIVE:
                context->Enabled = TRUE;
                break;
            case PSN_KILLACTIVE:
                context->Enabled = FALSE;
                break;
            case PSN_QUERYINITIALFOCUS:
                SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, (LPARAM)context->CountersLv);
                return TRUE;
            }

            PhHandleListViewNotifyForCopy(lParam, context->AppDomainsLv);
            PhHandleListViewNotifyForCopy(lParam, context->CountersLv);
        }
        break;
    case MSG_UPDATE:
        {
            if (context->Enabled && context->ControlBlockValid)
            {
                DotNetPerfUpdateCounterData(hwndDlg, context);
            }
        }
        break;
    case WM_SIZE:
        {
            ExtendedListView_SetColumnWidth(context->AppDomainsLv, 0, ELVSCW_AUTOSIZE_REMAININGSPACE);
        }
        break;
    }

    return FALSE;
}

VOID AddPerfPageToPropContext(
    _In_ PPH_PLUGIN_PROCESS_PROPCONTEXT PropContext
    )
{
    PhAddProcessPropPage(
        PropContext->PropContext,
        PhCreateProcessPropPageContextEx(PluginInstance->DllBase, MAKEINTRESOURCE(IDD_PROCDOTNETPERF), DotNetPerfPageDlgProc, NULL)
        );
}

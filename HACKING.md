## Building
System Informer must be built using a Microsoft C compiler. Do not attempt to use any other compiler or be prepared to spend a long time trying to fix things. The only tested IDE is Visual Studio 2015.

The Windows SDK 10 must be installed. To create a XP-compatible driver, KSystemInformer must be built using WDK v7, not the latest WDK.

## Conventions

### Names
* Functions, function parameters, and global variables use CamelCase.
* Local variables use lowerCamelCase.
* Structs, enums and unions use CAPS_WITH_UNDERSCORES.

All names must have an appropriate prefix (with some exceptions):

* "Ph" or "PH_" (structures) for public names.
* "Php" or "PHP_" for private names.
* Some variants such as "Pha".
* Private prefixes are created by appending "p" to the prefix. E.g. "Ph" -> "Php", "Pha" -> "Phap".
* Functions and global variables without a prefix must be declared "static".
  * "static" names must not have a public prefix.
  * Names with a private prefix do not have to be "static".
* Structures without a prefix must be declared in a ".c" file. Structures declared in a ".c" file may or may not have a prefix.

### Types
Unless used for the Win32 API, the standard types are:

* `BOOLEAN` for a 1 byte boolean, or `LOGICAL` for a 4 byte boolean.
* `UCHAR` for 1 byte.
* `SHORT`/`USHORT` for 2 bytes.
* `LONG`/`ULONG` for 4 bytes.
* `LONG64`/`ULONG64` for 8 bytes.
* `CHAR` for a 1 byte character.
* `WCHAR` for a 2 byte character.
* `PSTR` for a string of 1 byte characters.
* `PWSTR` for a string of 2 byte characters.

#### Booleans
Always use:

```
    if (booleanVariable) // not "if (booleanVariable == TRUE)"
    {
        ...
    }
```

to test a boolean value.

### Annotations, qualifiers
* All functions use SAL annotations, such as `_In_`, `_Inout_`, `_Out_`, etc.
* Do not use `const`, unless obvious optimizations can be made by the compiler (e.g. inlining).
* Do not use `volatile` in definitions. Instead, cast to a volatile pointer when necessary.

### Function success indicators
There are three main types of indicators used:

* A `NTSTATUS` value is returned. The `NT_SUCCESS` macro checks if a status value indicates success.
* A `BOOLEAN` value is returned. `TRUE` indicates success.
* The result of the function is returned (e.g. a pointer). A special value (e.g. `NULL`) indicates failure.

Unless indicated, a function which fails is guaranteed not to modify any of its output parameters (`_Out_`, `_Out_opt_`, etc.).

For functions which are passed a callback function, it is not guaranteed that a failed function has not executed the callback function.

### Threads
Every thread start routine must have the following signature:

```
    NTSTATUS NameOfRoutine(
        _In_ PVOID Parameter
        );
```

Thread creation is done through the `PhCreateThread` function.

### Collections
The collections available are summarized below:

Name                  | Use                     | Type
--------------------- | ----------------------- | ---------------
`PH_ARRAY`            | Array                   | Non-intrusive
`PH_LIST`             | Array                   | Non-intrusive
`LIST_ENTRY`          | Doubly linked list      | Intrusive
`SINGLE_LIST_ENTRY`   | Singly linked list      | Intrusive
`PH_POINTER_LIST`     | Array                   | Non-intrusive
`LIST_ENTRY`          | Stack                   | Intrusive
`SINGLE_LIST_ENTRY`   | Stack                   | Intrusive
`LIST_ENTRY`          | Queue                   | Intrusive
`RTL_AVL_TABLE`       | Binary tree (AVL)       | Non-intrusive
`PH_AVL_LINKS`        | Binary tree (AVL)       | Intrusive
`RTL_GENERIC_TABLE`   | Binary tree (splay)     | Non-intrusive
`PH_HASHTABLE`        | Hashtable               | Non-intrusive
`PH_HASH_ENTRY`       | Hashtable               | Intrusive
`PH_CIRCULAR_BUFFER`  | Circular buffer         | Non-intrusive

### Synchronization
The queued lock should be used for all synchronization, due to its small size and good performance. Although the queued lock is a reader-writer lock, it can be used as a mutex simply by using the exclusive acquire/release functions.

Events can be used through `PH_EVENT`. This object does not create a kernel event object until needed, and testing its state is very fast.

Rundown protection is available through `PH_RUNDOWN_PROTECT`.

Condition variables are available using the queued lock. Simply declare and initialize a queued lock variable, and use the `PhPulse(All)Condition` and `PhWaitForCondition` functions.

Custom locking with low overhead can be built using the wake event, built on the queued lock. Test one or more conditions in a loop and use `PhQueueWakeEvent`/`PhWaitForWakeEvent` to block. When a condition is modified use `PhSetWakeEvent` to wake waiters. If after calling `PhQueueWakeEvent` it is determined that no blocking should occur, use `PhSetWakeEvent`.

### Exceptions (SEH)
The only method of error handling used in Process Hacker is the return value (`NTSTATUS`, `BOOLEAN`, etc.). Exceptions are used for exceptional situations which cannot easily be recovered from (e.g. a lock acquire function fails to block, or an object has a negative reference count.

Exceptions to this rule include:

* `PhAllocate`, which raises an exception if it fails to allocate. Checking the return value of each allocation to increase reliability is not worth the extra effort involved, as failed allocations are very rare.
* `PhProbeAddress`, which raises an exception if an address lies outside of a specified range. Raising an exception makes it possible to conduct multiple checks in one SEH block.
* `STATUS_NOT_IMPLEMENTED` exceptions triggered by code paths which should not be reached, purely due to programmer error. `assert(FALSE)` could also be used in this case.

### Memory management
Use `PhAllocate`/`PhFree` to allocate/free memory. For complex objects, use the reference counting system.

There is semi-automatic reference counting available in the form of auto-dereference pools (similar to Apple's `NSAutoreleasePool`s). Use the `PhAutoDereferenceObject` to add an object to the thread's pool, and the object will be dereferenced at an unspecified time in the future. However, the object is guaranteed to not be dereferenced while the current function is executing.

Referencing an object is necessary whenever a pointer to the object is stored in a globally visible location or passed to another thread. In most other cases, referencing is not necessary.

All objects passed to functions must have a guaranteed reference for the duration of that call. One mistake is to keep a reference which could be destroyed in a window procedure, and to use that reference implicitly inside the window procedure. Messages can still be pumped (e.g. dialog boxes) while the window procedure is executing, so the window procedure must reference the object as soon as possible.

### Names (2)

#### Object creation/deletion
* Allocate means allocate memory without initialization.
* Create means allocate memory for an object and initialize the object.
* Free can be used for objects created with Allocate or Create functions.
* Destroy can also be used for objects created with Create functions.
* Initialize means initialize an object with caller-supplied storage.
* Delete is paired with Initialize and does not free the object as it was allocated by the caller.
* Create is used when objects are being created through the reference counting system.

Examples:
* `PhAllocateFromFreeList`/`PhFreeToFreeList`
* `PhCreateFileDialog`/`PhFreeFileDialog`
* `PhInitializeWorkQueue`/`PhDeleteWorkQueue`
* `PhCreateString`/`PhDereferenceObject`

#### Element counts
* Length specifies the length in bytes. E.g. `PhCreateString` requires the length to be specified in bytes.
* Count specifies the length in elements. E.g. `PhSubstring` requires the length to be specified in characters.
* Index specifies the index in elements. E.g. `PhSubstring` requires the index to be specified in characters.

When null terminated strings are being written to output, the return count, if any, must be specified as the number of characters written including the null terminator.

### Strings
Strings use the `PH_STRING` type, managed by reference counting. To create a string object from a null-terminated string:

```
    PPH_STRING myString = PhCreateString(L"My string");

    wprintf(
        L"My string is \"%s\", and uses %Iu bytes.\n",
        myString->Buffer,
        myString->Length
        );
```

All string objects have an embedded length (always in bytes), and the string is additionally null-terminated for compatibility reasons.

String objects must be treated as immutable unless a string object is created and modified before the pointer is shared with any other functions or stored in any global variables. This exception applies only when creating a string using `PhCreateString` or `PhCreateStringEx`.

Strings can be concatenated with `PhConcatStrings`:

```
    PPH_STRING newString;

    newString = PhConcatStrings(
        4,
        L"My first string, ",
        L"My second string, ",
        aStringFromSomewhere,
        L"My fourth string."
        );
```

Another version concatenates two strings:

```
    PPH_STRING newString;

    newString = PhConcatStrings2(
        L"My first string, ",
        L"My second string."
        );
```

Strings can be formatted:

```
    PPH_STRING newString;

    newString = PhFormatString(
        L"%d: %s, %#x",
        100,
        L"test",
        0xff
        );
```

### Tips
 * Use !! to "cast" to a boolean.

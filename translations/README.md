# Translation resources

System Informer keeps English strings compiled into each binary as the default and fallback language.
Optional translation DLLs are loaded from this directory as data-only PE resources; no code from a
translation DLL is executed. A missing DLL or string falls back to the built-in English text.

The initial proof project contains only the five top-level main-menu labels for `zh-CN`. Additional
strings and first-party plugin resources should be migrated in small, reviewable changes.

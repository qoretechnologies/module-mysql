MariaDB fixture resource audit
==============================

Copyright 2026 Qore Technologies, s.r.o.

Scope: RPM release 3 and private MariaDB resource settings. All three targets pass their runtime and SDK suites under a hard and soft nofile limit of 1024: eight helper cases, 34 database cases / 203 assertions and SDK qcc execution. The existing two approved server diagnostics remain narrowly checked.

.. list-table:: Complete audit-changes checklist
   :header-rows: 1

   * - Check
     - Status
     - Evidence

   * - 1. Entry exists in doxygen/lang/120_modules.dox.tmpl (for modules in the Qore repo; N/A for external module repos)
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 2. Entry exists in doxygen/lang/900_release_notes.dox.tmpl (for modules in the Qore repo; external modules have release notes in their .qm)
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 3. qore_user_module() or qore_external_user_module() call in CMakeLists.txt
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 4. Module added to QMOD list in CMakeLists.txt
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 5. .qm file has @section <lowercasemodname>intro as first doc section — must be all lowercase (e.g., avrodataproviderintro, not AvroDataProviderintro)
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 6. %modern in .qm file — no redundant %new-style, %require-types, %strict-args, %enable-all-warnings
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 7. No parse directives (%requires, %modern, %new-style) in separated .qc files (check OUTSIDE of @code blocks only)
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 8. No %include usage (deprecated for modules)
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 9. Copyright 2026 on all new files
     - Pass
     - Changed files retain 2026 copyright.

   * - 10. Directory layout: .qm inside qlib/<ModuleName>/ directory (not at qlib/<ModuleName>.qm for multi-file modules)
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 11. No second .qm for the same module at qlib/<ModuleName>.qm
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 12. ns=Qore::XX matches the QoreNamespace constructor path
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 13. %modern directive present
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 14. Executable permission set (chmod +x)
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 15. Uses %prepend-module-path  before %requires for in-repo modules (Qore and Qore modules only; not Qorus)
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 16. External module dependencies use %try-module — except modules delivered with the project itself (Qore ex: DataProvider, ConnectionProvider, QUnit, etc.) which use hard %requires
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 17. No filesystem operations (fopen, open, creat, unlink, remove, rename, mkdir, rmdir, stat, chmod) without sandbox checks
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 18. No network operations (connect, bind, socket, getaddrinfo, gethostbyname) without sandbox checks
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 19. If filesystem/network ops exist, verify QoreSandboxManagerHelper usage
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 20. No File::, Dir::, Socket::, HTTPClient:: usage without justification
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 21. All for/while loops that could iterate >100 times have qore_check_cancel() checks
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 22. Uses qore_check_cancel() (NOT deprecated qore_check_io_interrupt())
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 23. Check frequency: every 100 iterations for tight loops, every 10 for expensive iterations
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 24. No blocking operations without cancellation support
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 25. Every action has display_name, short_desc (plain text, <80 chars), desc (markdown)
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 26. Every action has options populated via getActionOptionFromFields() — without this, the action shows an empty, unusable form
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 27. Every action has output_type set to a typed data type constant (e.g., MyResponseDataType) — not omitted
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 28. DPAT_API actions: provider has "supports_request": True and implements doRequestImpl()
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 29. DPAT_FIND actions: every option exists in SearchOptions, getRecordTypeImpl() returns *hash<string, AbstractDataField>
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 30. Scheme-based apps (with "scheme" in registerApp): actions use "path" and do NOT use "cls" — having both scheme and cls causes a runtime error
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 31. Single-key hash slices use trailing comma: Fields{"key",} (without trailing comma, Fields{"key"} returns the value, not a hash)
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 32. Typed data type classes exist for request and response types — inherit HashDataType, have const Fields hash, call addQoreFields(Fields) in constructor, export public constant at bottom (e.g., public const MyDataType = new MyDataType();)
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 33. Request/input types use public Fields (enables ClassName::Fields in action registration)
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 34. Response/output types use private Fields
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 35. Each field in data types has display_name, type, and desc (markdown-formatted)
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 36. Input fields have example_value where useful (string fields, endpoint URIs, SQL queries, etc.)
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 37. Fields with finite allowed values use allowed_values with AllowedValueInfo containing both value and display_name (Title Case, human-readable) — never bare values, never described only in text
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 38. Password/secret fields have "sensitive": True
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 39. groups uses AppGroup enum values from qlib/DataProvider/AppGroup.qc
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 40. App logo stored as separate file, loaded at module level in Priv namespace
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 41. App desc uses markdown: bullet list of capabilities, links to project website, business-language explanation of value
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 42. display_name is user-friendly ("Apache Avro" not "avro")
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 43. short_desc is plain text, under 80 chars, single sentence — no markdown
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 44. desc uses markdown: backticks for code/field refs ( field_name ,  True ,  pdf ), \n\n for paragraphs, -  bullet lists for enumerations, bold for caveats
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 45. Descriptions use plain business language relating to common challenges — not just technical "what" but "why" and "when to use"
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 46. No bare True/False/NOTHING — must be backtick-wrapped in desc
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 47. No bare field/option names in prose — must use backticks
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 48. Long descriptions (>500 chars) use bold section headers and bullet lists
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 49. Factory registration in Qore repo: every factory name registered in qlib/DataProvider/DataProvider.qc → FactoryMap (without this, module loads but doesn't appear in Qorus apps)
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 50. getRecordTypeImpl() signature: must be private *hash<string, AbstractDataField> getRecordTypeImpl(*hash<auto> search_options) — NOT returning *AbstractDataProviderType
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 51. Dependency JARs committed (for JNI modules): JAR files in qlib/*/jar/ may be gitignored — use git add -f to ensure they're tracked, otherwise CI compilation fails
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 52. JAR install rules in CMakeLists.txt for all dependency JARs
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 53. No workarounds: No TODOs, FIXMEs, stubs, or partially-implemented features
     - Pass
     - Root cause: MariaDB calculates descriptors from connection count and twice the table cache times cache instances. Set bounded fixture values; warnings remain fatal.

   * - 54. Exception safety: C++ uses ReferenceHolder for Qore allocations, std::unique_ptr for C++ allocations, *xsink checked after every fallible operation
     - Pass
     - Existing context managers reap the private server and clean the temporary directory on every exit.

   * - 55. Thread safety: All mutable shared state protected by std::lock_guard<std::mutex> or documented as immutable-after-construction
     - Pass
     - Each test invocation owns its socket, data directory and server.

   * - 56. Type safety: Strongly-typed code<return(args)> instead of untyped code; static_cast instead of C casts; typed hashdecls for results; enums where appropriate
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 57. Performance: No O(n²) where O(n) is possible; no unnecessary copies; coordinate descent uses incremental residuals not full matrix multiply
     - Pass
     - One table-cache instance and 64 cached tables are sufficient for the isolated test database.

   * - 58. Error handling: All inputs validated (dimensions, empty data, unfitted models); C++ I/O handles EAGAIN/EINTR if applicable
     - Pass
     - The diagnostic unit test now explicitly rejects max_open_files warnings.

   * - 59. Documentation: Doxygen @param, @return, @throw on all public methods; @par Example with realistic business scenarios; @note for important caveats
     - Pass
     - RPM README and changelog document the constrained-worker qualification.

   * - 60. QPP flags: [flags=CONSTANT] on methods that never throw; [flags=RET_VALUE_ONLY] on methods that throw but have no side effects
     - N/A
     - No module definitions, native code, Qore tests, provider registrations or Java dependencies change in this packaging-only patch.

   * - 61. Security: No user-controlled format strings; no buffer overflows; bounds checking on array indices; no credentials in code
     - Pass
     - Private Unix socket only; no external server or deployment credentials.

   * - 62. Correctness: Algorithms verified against reference implementations; edge cases tested (empty data, single sample, all-zero features)
     - Pass
     - Six actual installed tests pass with --ulimit nofile=1024:1024 on Fedora 44, Leap 16.0 and EL10.

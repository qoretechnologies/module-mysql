MySQL RPM qualification review
==============================

Copyright 2026 Qore Technologies, s.r.o.

Scope: modern portable spec, private MariaDB fixture, fixture tests and packaging
instructions. No C++ or Qore runtime implementation is changed.

Candidate 4 passes on Fedora 44, Leap 16.0 and EL10: complete RPM builds, strict
HTML API reference, eight fixture tests and all three MySQL suites (34 cases /
203 assertions). Minimal installed runtime contains neither SDK nor compiler;
the SDK check also compiles and executes the parameterized database example.
The fixture permits only the two maintainer-approved MariaDB diagnostics described
in README.rst and rejects every other warning/error. Final canonical builds
against the latest qualified Qore SDK and native aarch64 remain release gates.

Evidence: qore-packaging/results/{target}-mysql-candidate-4/build.json and
{target}-mysql-installed-candidate-4.json with the referenced full logs.

.. list-table:: Full audit checklist
   :header-rows: 1

   * - Check
     - Status
     - Evidence

   * - 1. Entry exists in doxygen/lang/120_modules.dox.tmpl (for modules in the Qore repo; N/A for external module repos)
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 2. Entry exists in doxygen/lang/900_release_notes.dox.tmpl (for modules in the Qore repo; external modules have release notes in their .qm)
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 3. qore_user_module() or qore_external_user_module() call in CMakeLists.txt
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 4. Module added to QMOD list in CMakeLists.txt
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 5. .qm file has @section <lowercasemodname>intro as first doc section — must be all lowercase (e.g., avrodataproviderintro, not AvroDataProviderintro)
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 6. %modern in .qm file — no redundant %new-style, %require-types, %strict-args, %enable-all-warnings
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 7. No parse directives (%requires, %modern, %new-style) in separated .qc files (check OUTSIDE of @code blocks only)
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 8. No %include usage (deprecated for modules)
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 9. Copyright 2026 on all new files
     - Pass
     - All new RPM support files carry 2026 notices.

   * - 10. Directory layout: .qm inside qlib/<ModuleName>/ directory (not at qlib/<ModuleName>.qm for multi-file modules)
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 11. No second .qm for the same module at qlib/<ModuleName>.qm
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 12. ns=Qore::XX matches the QoreNamespace constructor path
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 13. %modern directive present
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 14. Executable permission set (chmod +x)
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 15. Uses %prepend-module-path  before %requires for in-repo modules (Qore and Qore modules only; not Qorus)
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 16. External module dependencies use %try-module — except modules delivered with the project itself (Qore ex: DataProvider, ConnectionProvider, QUnit, etc.) which use hard %requires
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 17. No filesystem operations (fopen, open, creat, unlink, remove, rename, mkdir, rmdir, stat, chmod) without sandbox checks
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 18. No network operations (connect, bind, socket, getaddrinfo, gethostbyname) without sandbox checks
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 19. If filesystem/network ops exist, verify QoreSandboxManagerHelper usage
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 20. No File::, Dir::, Socket::, HTTPClient:: usage without justification
     - Pass
     - Database I/O is restricted to a private mode-0700 temporary directory and Unix socket, with server TCP networking disabled.

   * - 21. All for/while loops that could iterate >100 times have qore_check_cancel() checks
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 22. Uses qore_check_cancel() (NOT deprecated qore_check_io_interrupt())
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 23. Check frequency: every 100 iterations for tight loops, every 10 for expensive iterations
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 24. No blocking operations without cancellation support
     - Pass
     - Readiness is driven by the server log event with a deadline, logs drain continuously and the process group is reaped on success and exceptions.

   * - 25. Every action has display_name, short_desc (plain text, <80 chars), desc (markdown)
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 26. Every action has options populated via getActionOptionFromFields() — without this, the action shows an empty, unusable form
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 27. Every action has output_type set to a typed data type constant (e.g., MyResponseDataType) — not omitted
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 28. DPAT_API actions: provider has "supports_request": True and implements doRequestImpl()
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 29. DPAT_FIND actions: every option exists in SearchOptions, getRecordTypeImpl() returns *hash<string, AbstractDataField>
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 30. Scheme-based apps (with "scheme" in registerApp): actions use "path" and do NOT use "cls" — having both scheme and cls causes a runtime error
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 31. Single-key hash slices use trailing comma: Fields{"key",} (without trailing comma, Fields{"key"} returns the value, not a hash)
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 32. Typed data type classes exist for request and response types — inherit HashDataType, have const Fields hash, call addQoreFields(Fields) in constructor, export public constant at bottom (e.g., public const MyDataType = new MyDataType();)
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 33. Request/input types use public Fields (enables ClassName::Fields in action registration)
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 34. Response/output types use private Fields
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 35. Each field in data types has display_name, type, and desc (markdown-formatted)
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 36. Input fields have example_value where useful (string fields, endpoint URIs, SQL queries, etc.)
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 37. Fields with finite allowed values use allowed_values with AllowedValueInfo containing both value and display_name (Title Case, human-readable) — never bare values, never described only in text
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 38. Password/secret fields have "sensitive": True
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 39. groups uses AppGroup enum values from qlib/DataProvider/AppGroup.qc
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 40. App logo stored as separate file, loaded at module level in Priv namespace
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 41. App desc uses markdown: bullet list of capabilities, links to project website, business-language explanation of value
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 42. display_name is user-friendly ("Apache Avro" not "avro")
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 43. short_desc is plain text, under 80 chars, single sentence — no markdown
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 44. desc uses markdown: backticks for code/field refs ( field_name ,  True ,  pdf ), \n\n for paragraphs, -  bullet lists for enumerations, bold for caveats
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 45. Descriptions use plain business language relating to common challenges — not just technical "what" but "why" and "when to use"
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 46. No bare True/False/NOTHING — must be backtick-wrapped in desc
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 47. No bare field/option names in prose — must use backticks
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 48. Long descriptions (>500 chars) use bold section headers and bullet lists
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 49. Factory registration in Qore repo: every factory name registered in qlib/DataProvider/DataProvider.qc → FactoryMap (without this, module loads but doesn't appear in Qorus apps)
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 50. getRecordTypeImpl() signature: must be private *hash<string, AbstractDataField> getRecordTypeImpl(*hash<auto> search_options) — NOT returning *AbstractDataProviderType
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 51. Dependency JARs committed (for JNI modules): JAR files in qlib/*/jar/ may be gitignored — use git add -f to ensure they're tracked, otherwise CI compilation fails
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 52. JAR install rules in CMakeLists.txt for all dependency JARs
     - N/A
     - No Qore/native module implementation, Qore test import, class, DataProvider, cancellation loop or Java dependency added.

   * - 53. No workarounds: No TODOs, FIXMEs, stubs, or partially-implemented features
     - Pass
     - Explicit MariaDB provider selection and address configuration match the actual standalone fixture. The two remaining external diagnostics have explicit maintainer approval; all others fail qualification.

   * - 54. Exception safety: C++ uses ReferenceHolder for Qore allocations, std::unique_ptr for C++ allocations, *xsink checked after every fallible operation
     - Pass
     - Temporary directories, pipes and log readers are context-managed. Eight unit tests cover early exit, timeout, cleanup, full pipes, shutdown escalation, diagnostics and native-module isolation.

   * - 55. Thread safety: All mutable shared state protected by std::lock_guard<std::mutex> or documented as immutable-after-construction
     - Pass
     - The single reader thread exclusively drains the server pipe after startup; teardown collects it after server termination.

   * - 56. Type safety: Strongly-typed code<return(args)> instead of untyped code; static_cast instead of C casts; typed hashdecls for results; enums where appropriate
     - Pass
     - Subprocess argument lists avoid shell interpolation; module discovery rejects zero or multiple native artifacts.

   * - 57. Performance: No O(n²) where O(n) is possible; no unnecessary copies; coordinate descent uses incremental residuals not full matrix multiply
     - Pass
     - One private server per complete test run; no network service or database added to runtime dependencies.

   * - 58. Error handling: All inputs validated (dimensions, empty data, unfitted models); C++ I/O handles EAGAIN/EINTR if applicable
     - Pass
     - Connection preflight prevents missing-server skips. Diagnostic validation rejects duplicates, changed accounts/messages, missing authentication coverage and every unexpected warning/error.

   * - 59. Documentation: Doxygen @param, @return, @throw on all public methods; @par Example with realistic business scenarios; @note for important caveats
     - Pass
     - README gives source/build/installed/compiler commands and explains source exclusions, DBI metadata behavior and the approved diagnostics; strict Doxygen passes.

   * - 60. QPP flags: [flags=CONSTANT] on methods that never throw; [flags=RET_VALUE_ONLY] on methods that throw but have no side effects
     - N/A
     - No QPP flags or native runtime implementation changed.

   * - 61. Security: No user-controlled format strings; no buffer overflows; bounds checking on array indices; no credentials in code
     - Pass
     - No real credentials used or system database touched. Private socket-only passwordless test account; MariaDB LGPL linkage and source-license exclusion verified.

   * - 62. Correctness: Algorithms verified against reference implementations; edge cases tested (empty data, single sample, all-zero features)
     - Pass
     - Candidate 4 passes full builds on three distributions, eight fixture tests, 34 Qore cases / 203 assertions, minimal installed runtime and parameterized compiled-application checks.

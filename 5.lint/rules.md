### always-comb
Checks that there are no occurrences of `always @*`. Use `always_comb` instead. See [Style: combinational-logic].

Enabled by default: true

### always-comb-blocking
Checks that there are no occurrences of non-blocking assignment in combinational logic. See [Style: combinational-logic].

Enabled by default: true

### always-ff-non-blocking
Checks that blocking assignments are, at most, targeting locals in sequential logic. See [Style: sequential-logic].

##### Parameters
  * `catch_modifying_assignments` Default: `false` 
  * `waive_for_locals` Default: `false` 

Enabled by default: true

### banned-declared-name-patterns
Checks for banned declared name against set of unwanted patterns. See [Style: identifiers].

Enabled by default: false

### case-missing-default
Checks that a default case-item is always defined unless the case statement has the `unique` qualifier. See [Style: case-statements].

Enabled by default: true

### constraint-name-style
Check that constraint names follow the lower_snake_case convention and end with _c. See [Style: constraints].

Enabled by default: true

### create-object-name-match
Checks that the 'name' argument of `type_id::create()` matches the name of the variable to which it is assigned. See [Style: uvm-naming].

Enabled by default: true

### disable-statement
Checks that there are no occurrences of `disable some_label` if label is referring to a fork or other none sequential block label. Use `disable fork` instead. See [Style: disable-invalid-in-non-sequential].

Enabled by default: false

### endif-comment
Checks that a Verilog `` `endif`` directive is followed by a comment that matches the name of the opening `` `ifdef`` or `` `ifndef``. See [Style: endif-comment].

Enabled by default: false

### enum-name-style
Checks that `enum` names use lower_snake_case naming convention and end with '_t' or '_e'. See [Style: enumerations].

Enabled by default: true

### explicit-function-lifetime
Checks that every function declared outside of a class is declared with an explicit lifetime (static or automatic). See [Style: function-task-explicit-lifetime].

Enabled by default: true

### explicit-function-task-parameter-type
Checks that every function and task parameter is declared with an explicit storage type. See [Style: function-task-argument-types].

Enabled by default: true

### explicit-parameter-storage-type
Checks that every `parameter` and `localparam` is declared with an explicit storage type. See [Style: constants].

##### Parameter
  * `exempt_type` Default: `` Set to `string` to exempt string types

Enabled by default: true

### explicit-task-lifetime
Checks that every task declared outside of a class is declared with an explicit lifetime (static or automatic). See [Style: function-task-explicit-lifetime].

Enabled by default: true

### forbid-consecutive-null-statements
Checks that there are no occurrences of consecutive null statements like `;;` See [Style: redundant-semicolons].

Enabled by default: true

### forbid-defparam
Do not use defparam. See [Style: module-instantiation].

Enabled by default: true

### forbid-line-continuations
Checks that there are no occurrences of `\` when breaking the string literal line. Use concatenation operator with braces instead. See [Style: forbid-line-continuations].

Enabled by default: true

### forbid-negative-array-dim
Check for negative constant literals inside array dimensions. See [Style: forbid-negative-array-dim].

Enabled by default: false

### forbidden-macro
Checks that no forbidden macro calls are used. See [Style: uvm-logging].

Enabled by default: true

### generate-label
Checks that every generate block statement is labeled. See [Style: generate-statements].

Enabled by default: true

### generate-label-prefix
Checks that every generate block label starts with g_ or gen_. See [Style: generate-constructs].

Enabled by default: true

### interface-name-style
Checks that `interface` names use lower_snake_case naming convention and end with `_if`. See [Style: interface-conventions].

Enabled by default: true

### invalid-system-task-function
Checks that no forbidden system tasks or functions are used. These consist of the following functions: `$psprintf`, `$random`, and `$dist_*`. As well as non-LRM function `$srandom`. See [Style: forbidden-system-functions].

Enabled by default: true

### legacy-generate-region
Checks that there are no generate regions. See [Style: generate-constructs].

Enabled by default: false

### legacy-genvar-declaration
Checks that there are no separate `genvar` declarations. See [Style: generate-constructs].

Enabled by default: false

### line-length
Checks that all lines do not exceed the maximum allowed length.  See [Style: line-length].

##### Parameter
  * `length` Default: `100` Desired line length

Enabled by default: true

### macro-name-style
Checks that every macro name follows ALL_CAPS naming convention. _Exception_: UVM-like macros. See [Style: defines].

Enabled by default: true

### macro-string-concatenation
Concatenation will not be evaluated here. Use `"...`" instead. See [Style: defines].

Enabled by default: false

### mismatched-labels
Check for matching begin/end labels. See [Style: mismatched-labels].

Enabled by default: false

### module-begin-block
Checks that there are no begin-end blocks declared at the module level. See [Style: floating-begin-end-blocks].

Enabled by default: true

### module-filename
If a module is declared, checks that at least one module matches the first dot-delimited component of the file name. Depending on configuration, it is also allowed to replace underscore with dashes in filenames. See [Style: file-names].

##### Parameter
  * `allow-dash-for-underscore` Default: `false` Allow dashes in the filename where there are dashes in the module name

Enabled by default: true

### module-parameter
Checks that module instantiations with more than one parameter are passed in as named parameters, rather than positional parameters. See [Style: module-instantiation].

Enabled by default: true

### module-port
Checks that module instantiations with more than one port are passed in as named ports, rather than positional ports. See [Style: module-instantiation].

Enabled by default: true

### no-tabs
Checks that no tabs are used. Spaces should be used instead of tabs.  See [Style: tabs].

Enabled by default: true

### no-trailing-spaces
Checks that there are no trailing spaces on any lines. See [Style: trailing-spaces].

Enabled by default: true

### numeric-format-string-style
Checks that string literals with numeric format specifiers have proper prefixes for hex and bin values and no prefixes for decimal values. See [Style: number-formatting].

Enabled by default: false

### one-module-per-file
Checks that at most one module is declared per file. See [Style: file-extensions].

Enabled by default: false

### package-filename
Checks that the package name matches the filename. Depending on configuration, it is also allowed to replace underscore with dashes in filenames. See [Style: file-names].

##### Parameter
  * `allow-dash-for-underscore` Default: `false` Allow dashes in the filename corresponding to the underscores in the package

Enabled by default: true

### packed-dimensions-range-ordering
Checks that packed dimension ranges are declare in little-endian (decreasing) order, e.g. `[N-1:0]`. See [Style: packed-ordering].

Enabled by default: true

### parameter-name-style
Checks that non-type parameter and localparam names follow at least one of the naming conventions from a choice of CamelCase and ALL_CAPS, ORed together with the pipe-symbol(|). Empty configuration: no style enforcement. See [Style: constants].

##### Parameters
  * `localparam_style` Default: `CamelCase` Style of localparam name
  * `parameter_style` Default: `CamelCase|ALL_CAPS` Style of parameter names

Enabled by default: true

### parameter-type-name-style
Checks that parameter type names follow the lower_snake_case naming convention and end with _t. See [Style: parametrized-objects].

Enabled by default: false

### plusarg-assignment
Checks that plusargs are always assigned a value, by ensuring that plusargs are never accessed using the `$test$plusargs` system task. See [Style: plusarg-value-assignment].

Enabled by default: true

### port-name-suffix
Check that port names end with _i for inputs, _o for outputs and _io for inouts. Alternatively, for active-low signals use _n[io], for differential pairs use _n[io] and _p[io]. See [Style: suffixes-for-signals-and-types].

Enabled by default: false

### positive-meaning-parameter-name
Checks that no parameter name starts with 'disable', using positive naming (starting with 'enable') is recommended. See [Style: binary-parameters].

Enabled by default: true

### posix-eof
Checks that the file ends with a newline. See [Style: posix-file-endings].

Enabled by default: true

### proper-parameter-declaration
Checks that every `parameter` declaration is inside a formal parameter list of modules/classes and every `localparam` declaration is inside a module, class or package. See [Style: constants].

##### Parameters
  * `package_allow_parameter` Default: `false` Allow parameters in packages (treated as a synonym for localparam).
  * `package_allow_localparam` Default: `true` Allow localparams in packages.

Enabled by default: false

### signal-name-style
Checks that signal names use lower_snake_case naming convention. Signals are defined as "a net, variable, or port within a SystemVerilog design". See [Style: signal-conventions].

Enabled by default: false

### struct-union-name-style
Checks that `struct` and `union` names use lower_snake_case naming convention and end with '_t'. See [Style: struct-union-conventions].

##### Parameter
  * `exceptions` Default: `` Comma separated list of allowed upper-case elements, such as unit-names

Enabled by default: true

### suggest-parentheses
Recommend extra parentheses around subexpressions where it helps readability. See [Style: parentheses].

Enabled by default: true

### suspicious-semicolon
Checks that there are no suspicious semicolons that might affect code behaviour but escape quick visual inspection See [Style: bugprone].

Enabled by default: false

### truncated-numeric-literal
Checks that numeric literals are not longer than their stated bit-width to avoid undesired accidental truncation. See [Style: number-literals].

Enabled by default: true

### typedef-enums
Checks that a Verilog `enum` declaration is named using `typedef`. See [Style: typedef-enums].

Enabled by default: true

### typedef-structs-unions
Checks that a Verilog `struct` or `union` declaration is named using `typedef`. See [Style: typedef-structs-unions].

##### Parameter
  * `allow_anonymous_nested` Default: `false` Allow nested structs/unions to be anonymous.

Enabled by default: true

### undersized-binary-literal
Checks that the digits of binary literals for the configured bases match their declared width, i.e. has enough padding prefix zeros. See [Style: number-literals].

##### Parameters
  * `bin` Default: `true` Checking binary 'b literals.
  * `oct` Default: `false` Checking octal 'o literals.
  * `hex` Default: `false` Checking hexadecimal 'h literals.
  * `lint_zero` Default: `false` Also generate a lint warning for value zero such as `32'h0`; autofix suggestions would be to zero-expand or untype `'0`.
  * `autofix` Default: `true` Provide autofix suggestions, e.g. 32'hAB provides suggested fix 32'h000000AB.

Enabled by default: true

### unpacked-dimensions-range-ordering
Checks that unpacked dimension ranges are declared in big-endian order `[0:N-1]`, and when an unpacked dimension range is zero-based `[0:N-1]`, the size is declared as `[N]` instead. See [Style: unpacked-ordering].

Enabled by default: true

### uvm-macro-semicolon
Checks that no `uvm_* macro calls end with ';'. See [Style: uvm-macro-semicolon-convention].

Enabled by default: false

### v2001-generate-begin
Checks that there are no generate-begin blocks inside a generate region. See [Style: generate-constructs].

Enabled by default: true

### void-cast
Checks that void casts do not contain certain function/method calls.  See [Style: void-casts].

Enabled by default: true


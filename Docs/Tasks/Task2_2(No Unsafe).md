### **Task: Replace Unsafe C Functions and Optimize Code**

#### **Objective**
Replace **unsafe C functions** (e.g., `scanf`, `printf`, `gets`, `strcpy`) with **safer alternatives** and ensure the code follows **best practices for performance and security**. While the [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines) are primarily for C++, many principles apply to C as well.

---

#### **Key Risks with Unsafe Functions**
- **Buffer overflows**: `scanf`, `gets`, `strcpy`, `strcat`.
- **Format string vulnerabilities**: `printf` with user-controlled input.
- **Undefined behavior**: Unchecked pointer arithmetic, integer overflows.

---

#### **Action Items**

1. **Replace Unsafe Functions**:
   - Use **safer alternatives** from the C standard library or custom wrappers:
     | Unsafe Function | Safer Alternative |
     |------------------|-------------------|
     | `scanf`          | `fgets` + `sscanf` |
     | `printf`         | `snprintf`        |
     | `gets`           | `fgets`           |
     | `strcpy`         | `strncpy`         |
     | `strcat`         | `strncat`         |
     | `sprintf`        | `snprintf`        |

   - Example:
     ```c
     // Unsafe:
     char buffer[100];
     scanf("%s", buffer);  // Risk of buffer overflow

     // Safer:
     fgets(buffer, sizeof(buffer), stdin);
     ```

2. **Validate Inputs**:
   - Always check return values and bounds:
     ```c
     if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
       // Handle error
     }
     ```

3. **Optimize for Performance**:
   - **Compiler optimizations**: Use `-O2` or `-O3` flags in `gcc`/`clang`.
   - **Avoid unnecessary copies**: Pass pointers instead of large structs.
   - **Use `restrict` keyword** for pointer aliases in performance-critical loops.
   - **Profile-guided optimization (PGO)**: Use `-fprofile-generate` and `-fprofile-use`.

4. **Static Analysis**:
   - Use tools like `clang-tidy`, `cppcheck`, or `-Wall -Wextra -Werror` in `gcc` to catch unsafe patterns.

5. **Document Safe Practices**:
   - Add a `SAFE_CODING.md` file with guidelines for the team, e.g.:
     - Always use bounds-checked functions.
     - Validate user input.
     - Avoid global variables.

---

#### **Performance Tips**
- **Inline small functions**: Use `static inline` for frequently called functions.
- **Loop unrolling**: Manually unroll loops for critical sections.
- **Cache-aware coding**: Optimize data structures for locality (e.g., structure of arrays → array of structures).

---
**Question for the team**:
- Should we focus on refactoring specific modules first, or apply these changes globally?
- Do we want to add automated static analysis to our CI pipeline?
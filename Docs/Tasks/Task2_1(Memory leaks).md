### **Task: Detect and Fix Memory Leaks**

#### **Objective**
Identify and resolve **memory leaks** and **security vulnerabilities** in our C codebase. Memory leaks can lead to:

- **Performance degradation** (gradual memory exhaustion).
- **Crashes** or undefined behavior.
- **Security risks** (e.g., sensitive data exposure).

We need to ensure all dynamically allocated memory is properly **freed** and **accessed safely**.

---

#### **Tools for Detection**
Use the following tools to automate leak detection:

1. **Valgrind** (Linux/MacOS):
   - Runs your program in a simulated environment and reports leaks, invalid accesses, and uninitialized values.
   - **Installation**:
     ```bash
     sudo apt-get install valgrind  # Debian/Ubuntu
     brew install valgrind          # MacOS
     ```
   - **Usage**:
     ```bash
     valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./your_program
     ```
   - **Focus on**:
     - "Definitely lost" and "Indirectly lost" blocks.
     - Invalid reads/writes (e.g., use-after-free, buffer overflows).

2. **AddressSanitizer (ASan)** (GCC/Clang):
   - A fast, compiler-based tool for detecting memory errors.
   - **Usage**:
     ```bash
     gcc -fsanitize=address -g your_program.c -o your_program
     ./your_program
     ```
   - **Pros**: Faster than Valgrind, integrates with CI/CD.

3. **Static Analysis**:
   - Use `clang-tidy` or `cppcheck` to analyze code for potential leaks without running it:
     ```bash
     cppcheck --enable=all your_program.c
     ```

---

#### **Action Items**

1. **Run Leak Detection**:
   - Test the entire codebase with **Valgrind** and **ASan**.
   - Document all reported leaks and errors in a shared file (e.g., `MEMORY_ISSUES.md`).

2. **Fix Leaks**:
   - For each leak:
     - Trace the allocation (e.g., `malloc`, `calloc`) and ensure a corresponding `free`.
     - Check for **double-frees** or **use-after-free** bugs.
   - Example:
     ```c
     void example_function() {
       int *arr = malloc(10 * sizeof(int));
       if (!arr) { /* Handle error */ }
       // ... use arr ...
       free(arr);  // Ensure this is called on all code paths!
     }
     ```

3. **Prevent Future Leaks**:
   - Adopt **defensive programming**:
     - Set pointers to `NULL` after freeing.
     - Use wrappers or macros for `malloc`/`free` to track allocations.
   - Consider **RAII-like patterns** (e.g., pairing `malloc`/`free` in the same function).

4. **Automate Testing**:
   - Add a **CI/CD step** to run Valgrind/ASan on every commit (e.g., GitHub Actions).

---

#### **Common Pitfalls**
- **Mismatched allocation/deallocation**: `malloc` + `delete` (C++) or `new` + `free` (C).
- **Leaks in error paths**: Always `free` before `return` on errors.
- **Global variables**: Ensure they are initialized and cleaned up.

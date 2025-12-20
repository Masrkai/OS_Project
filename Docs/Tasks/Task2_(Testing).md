### **Task: Implement Unit Testing with Criterion**

#### **Objective**
Ensure robust **test coverage** for our C project, focusing on:
- **Functional correctness** (unit tests).
- **Memory safety** (leaks, invalid accesses).

**Note**: Memory leak testing is detailed in **[Task2_1.md](Task2_1(Testing).md)**. This task covers **general unit testing**.

---

#### **Tools & Framework**
- **Criterion**: A modern, lightweight C testing framework.
  - **Why Criterion?**
    - Designed for C, avoiding C++ dependencies.
    - Supports assertions, parameterized tests, and TAP output.
    - Easy integration with CI/CD pipelines.

---

#### **Action Items**

1. **Set Up Criterion**:
   - Install Criterion for your development environment:
     ```bash
     git clone https://github.com/Snaipe/Criterion.git
     cd Criterion
     mkdir build && cd build
     cmake .. && make
     sudo make install
     ```
   - **Integrate with Makefile**:
     Add the following to your `Makefile`:
     ```makefile
     CFLAGS += $(shell pkg-config --cflags criterion)
     LDFLAGS += $(shell pkg-config --libs criterion)
     ```
     Example target for tests:
     ```makefile
     tests: your_tests
     your_tests: your_tests.o your_code.o
         gcc -o your_tests your_tests.o your_code.o $(LDFLAGS)
     ```

2. **Write Unit Tests**:
   - **Target critical functions**: Focus on core logic, edge cases, and error handling.
   - **Example test structure**:
     ```c
     #include <criterion/criterion.h>
     #include "your_code.h"  // Include your C headers

     Test(ExampleTestSuite, TestFunction) {
         int result = your_function(2, 3);
         cr_assert_eq(result, 5, "Expected 5, got %d", result);
     }
     ```
   - **Avoid testing implementation details**: Test behavior, not internal logic.

3. **Integrate with CI/CD** (if applicable):
   - Add a test script to your pipeline to run Criterion tests automatically on pushes/pull requests.

4. **Document Test Cases**:
   - Add a `README` or comment block in test files to explain:
     - What is being tested?
     - How to run the tests?
     - Expected outcomes.

---

#### **Test Coverage Goals**
- **Minimum**: 80% coverage for critical paths.
- **Tools under discussion**: `gcov`, `lcov`, or `gcovr` for coverage measurement.
- **Prioritize**:
  - Input validation.
  - Boundary conditions (e.g., empty inputs, max values).
  - Error handling (e.g., `NULL` pointers, file I/O failures).

---

#### **Resources**
- [Criterion Documentation](https://criterion.readthedocs.io/)
- [Criterion GitHub](https://github.com/Snaipe/Criterion)
- [Memory Leak Testing (Task2_1.md)](Task2_1(Testing).md)

---
**Questions for the Team**:
- Should we start with a specific module for testing, or does everyone have a preference?
- Do we need a shared template for test files?
- Should we proceed with `gcov`/`lcov`/`gcovr` for coverage, or explore alternatives?
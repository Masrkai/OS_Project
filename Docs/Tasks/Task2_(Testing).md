### **Task: Implement Unit Testing with Google Test (gtest)**

#### **Objective**
Ensure robust **test coverage** for our project, focusing on:

- **Functional correctness** (unit tests).
- **Memory safety** (leaks, invalid accesses).

**Note**: Memory leak testing is detailed in **[Task2_1.md](Task2_1(Testing).md)**. This task covers **general unit testing**.

---

#### **Tools & Framework**
- **Google Test (gtest)**: A C++ testing framework (compatible with C projects via wrappers).
  - **Why gtest?**
    - Widely used, well-documented, and integrates with CI/CD pipelines.
    - Supports assertions, parameterized tests, and death tests.

---

#### **Action Items**

1. **Set Up gtest**:
   - Install gtest for your development environment:
     ```bash
     git clone https://github.com/google/googletest.git
     cd googletest
     mkdir build && cd build
     cmake .. && make
     sudo make install
     ```
   - Link gtest to your C project (example `CMakeLists.txt` configuration available [here](https://google.github.io/googletest/quickstart-cmake.html)).

2. **Write Unit Tests**:
   - **Target critical functions**: Focus on core logic, edge cases, and error handling.
   - **Example test structure**:
     ```cpp
     #include <gtest/gtest.h>
     extern "C" {
       #include "your_code.h"  // Include your C headers
     }

     TEST(ExampleTestSuite, TestFunction) {
       int result = your_function(2, 3);
       ASSERT_EQ(result, 5);
     }
     ```
   - **Avoid testing implementation details**: Test behavior, not internal logic.

3. **Integrate with CI/CD** (if applicable):
   - Add a test script to your pipeline to run gtest automatically on pushes/pull requests.

4. **Document Test Cases**:
   - Add a `README` or comment block in test files to explain:
     - What is being tested?
     - How to run the tests?
     - Expected outcomes.

---

#### **Test Coverage Goals**
- **Minimum**: 80% coverage for critical paths (use `gcov`/`lcov` to measure).
- **Prioritize**:
  - Input validation.
  - Boundary conditions (e.g., empty inputs, max values).
  - Error handling (e.g., `NULL` pointers, file I/O failures).

---

#### **Resources**
- [Google Test Documentation](https://google.github.io/googletest/)
- [gtest + C Example](https://stackoverflow.com/questions/12703739/how-to-use-googletest-with-c-code)
- [Memory Leak Testing (Task2_1.md)](Task2_1(Testing).md)

---
**Question for the team**:

- Should we start with a specific module for testing, or does everyone have a preference?
- Do we need a shared template for test files?
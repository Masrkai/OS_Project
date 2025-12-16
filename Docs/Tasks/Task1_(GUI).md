### **Task: GUI/WebUI Development with Sciter (C Integration)**

#### **Background**
As discussed with **Sama Okasha (TA)**, our project requires a **GUI/WebUI**. Given that our codebase is in **C**, traditional GUI frameworks (like Qt) are not natively compatible. After research, **Sciter** emerged as a viable solution:

- Sciter allows us to build GUIs using **HTML, CSS, and JavaScript**, while embedding the UI in a C application.
- It bridges the gap between web technologies and native applications.

#### **Challenges**
- **No prior experience**: None of us have worked with **HTML, CSS, or Sciter** before.
- **Limited C documentation**: Most Sciter tutorials/examples are for **C++** ([example](https://sciter.com/hello-cpp-tutorial/)), not C.
- **Fallback plan**: If Sciter proves too complex, we’ll default to **Qt (C++)**, though this also requires adapting to C++.

#### **Goals**
1. **Minimal viable UI**: Use **only HTML and CSS** (avoid JavaScript for simplicity).
2. **Proof of concept**: Build a basic UI (e.g., a window with buttons/text fields) to validate Sciter’s feasibility with C.
3. **Document the process**: Share learnings/roadblocks for future reference.

#### **Action Items**
1. **Setup Sciter for C**:
   - Download the [Sciter SDK](https://sciter.com/download/) and integrate it with our C project.
   - Follow the [C++ tutorial](https://sciter.com/hello-cpp-tutorial/) as a reference, adapting it for C.

2. **Design a simple UI**:
   - Create a basic HTML/CSS template (e.g., a form or dashboard).
   - Use Sciter’s API to render the UI from C.

3. **Test and iterate**:
   - Verify UI responsiveness and functionality.
   - Debug integration issues (e.g., event handling without JS).

4. **Fallback plan**:
   - If Sciter fails, research **Qt with C++** and outline migration steps.

#### **Resources**
- [Sciter Documentation](https://sciter.com/docs/)
- [HTML/CSS Crash Course (W3Schools)](https://www.w3schools.com/html/)
- [Qt for C++ (Backup)](https://www.qt.io/)

#### **Notes**
- This is **experimental**—expect trial and error.
- Prioritize **simplicity** and **documentation** to ease collaboration.

---
**Question for the team**: Should we schedule a quick sync to align on the UI design or Sciter setup? Or does anyone have prior experience with these tools?
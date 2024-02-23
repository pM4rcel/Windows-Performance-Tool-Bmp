## Computer Performance Evaluation Application

---

**Description**

This project implements a graphical application to evaluate personal computer performance through analysis of BMP images. The application supports grayscale conversion and byte inversion operations. Performance is assessed using sequential, static parallel, and dynamic parallel implementations.

**User Interface:**

* Display of computer information (SID, HT, NUMA, CPU Sets)
* Loaded image header details
* Test selection
* Execution time display
* Buttons for image operations and test execution
* Information from the file header and bitmap header of the file loaded by user

**Functionality:**
* Measure execution times for grayscale conversion and byte inversion (all implementation methods)
* Save processed images to separate files with encoded names (including timestamps)
* Save computer specifications and timestamps to text files
* Compare and verify results with sequential execution
* Generate performance tables and graphs

**Technical Details:**

* **Images:** BMP format, 32-bits/pixel (RGBA), No compression
* **Grayscale Formula:** 0.299 * R + 0.587 * G + 0.114 * B
* **Parallelization:**
    * Static: Equal iteration distribution across threads
    * Dynamic: Coordinator-worker model with dynamic chunk sizes

**Development Environment**

* **C++ Standard:** C++20
* **IDE:** Visual Studio Community 2022
* **Tools:** ReSharper (JetBrains)

You can check [here](https://docs.google.com/presentation/d/1hGtyquhGqGlUQ0FiuF26qmOuz9mQinT19oiCTvUx4ic/edit?usp=sharing) to see a presentation of this project.







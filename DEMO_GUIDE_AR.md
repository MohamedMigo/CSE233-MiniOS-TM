# دليل التسليم والمناقشة - Phase 1 (MiniOS-TM)

## قبل التسليم

1. افتح Ubuntu أو Ubuntu على WSL2 (مش Windows CMD / PowerShell لوحده).
2. فك ملف ZIP ثم افتح Terminal داخل المجلد.
3. نفّذ `make` ثم `make test` ثم `./minios_tm`.
4. خذ Screenshots حقيقية من Terminal بتاعك، خصوصًا الأربع حالات في مجلد `evidence`.
5. ارفع ZIP واحد فقط من حساب Team Leader على Canvas قبل الموعد المحدد.

## العرض العملي السريع

**Demo 1 - Foreground**

- اختار `1`، ثم اكتب `echo MiniOS_OK`، ثم `1` للـ Burst، ثم `64` للـ Memory.
- اختار `3` علشان تعرض الـ Task Table.
- اشرح: `fork()` بيعمل Child، و`getpid()/getppid()` بيعرضوا العلاقة، و`execvp()` بيشغّل `echo`، و`waitpid()` بيخلي الـ Parent يستنى النتيجة.

**Demo 2 - Background**

- اختار `2`، ثم `sleep 30`، ثم `30`، ثم `256`.
- اختار `3`: العملية RUNNING والـ Parent لسه قادر يستقبل أوامر.
- اشرح: `waitpid(..., WNOHANG)` بيراجع الحالة من غير ما يوقف الـ Parent.

**Demo 3 - Termination**

- اختار `4` وأدخل Task ID الظاهر للـ `sleep 30`.
- اختار `3`: هتلاقي حالة العملية TERMINATED و `SIG15`.
- اشرح: `kill(pid, SIGTERM)` بيبعت إشارة، وبعدها `waitpid()` بيجمع حالة خروج الـ Child علشان ما يفضلش Zombie.

**Demo 4 - Background completes by itself**

- اختار `2`، واكتب `sleep 2`، ثم `2` و `128`.
- بعد حوالي 3 ثواني اختار `3`، وهتلاقي COMPLETED.
- اختار `5` للخروج وتنظيف أي Process لسه شغال.

## أسئلة مهمة جدًا للدكتور

**Q1: What is the difference between fork() and execvp()?**  
`fork()` creates a new child process. `execvp()` replaces the program running in that child with a Linux command.

**Q2: What are getpid() and getppid()?**  
`getpid()` returns the current process ID. `getppid()` returns its parent process ID.

**Q3: What is the difference between foreground and background tasks?**  
Foreground waits until completion. Background allows the parent to continue accepting commands.

**Q4: Why use waitpid() instead of sleep()?**  
`waitpid()` waits for a *specific child* and collects its real exit status. `sleep()` only pauses for a time.

**Q5: What does WNOHANG mean?**  
It makes `waitpid()` return immediately if the child is still running instead of blocking the parent.

**Q6: What is a zombie process?**  
A child that has finished but whose parent has not yet collected the exit status. We call `waitpid()` to reap completed children.

**Q7: What does kill(pid, SIGTERM) do?**  
It requests graceful termination of the selected process. Then the parent calls `waitpid()` and records the final status.

**Q8: Why is this part of the approved project?**  
Our MiniOS-TM manages real Linux tasks through a CLI and TaskPCB. The process management module is the base for IPC, threads, scheduling and memory in future phases.

**Q9: Does the Burst Time change actual CPU scheduling now?**  
No. In Phase 1 it is task metadata. Actual scheduling will be developed in later phases.

**Q10: Does Phase 1 support real-time continuous process monitoring?**  
It checks background task completion on each menu refresh or new action, plus cleanup. A completed child may temporarily wait for the next check; full asynchronous reaping is not implemented here.

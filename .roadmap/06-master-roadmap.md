# Master Roadmap: ReactOS -> Windows 7

> Tổng hợp 5 báo cáo R1-R5 thành lộ trình thực thi.
> Nguồn: `00-missing-components.md`, `01-wip-audit.md`, `02-kernel-api-gaps.md`,
> `03-usermode-api-gaps.md`, `04-wddm-research.md`, `05-modern-stack-research.md`.
> Ngày: 2026-05-29. Phạm vi snapshot: `F:/reactos` branch `main`.

---

## 1. Mục tiêu & Phạm vi

### In scope (mục tiêu phải đạt)
- **Client Windows 7 SP1 x86 / x64** (NT 6.1) chạy ổn định trong VM (VBox, QEMU, VMware).
- **API surface Win7 user-mode**: kernel32 Vista/Win7, kernelbase + api-set schema, Thread Pool,
  Locale Ex, Condition Variables, SRW Locks, FileInformationByHandleEx, ETW user-mode.
- **Shell Win7**: Superbar/Taskbar + Jump Lists + ITaskbarList3/4, Libraries (IShellLibrary),
  IFileDialog modern, KnownFolders Win7, Aero Basic theme.
- **DWM software path** (compositor không yêu cầu hardware-accelerated WDDM thật) để bật Aero Basic.
- **WASAPI + audiodg.exe** đủ để phát/thu shared-mode + per-app volume slider.
- **UAC + MIC** end-to-end: split token logon, consent.exe, appinfo service, SRM integrity check, UIPI.
- **BCD boot path** đọc-ghi: bootmgr fork từ FreeLdr + winload tương thích LOADER_PARAMETER_BLOCK Vista/Win7.
- **App compat**: chạy được .NET 4.0/4.5, Office 2010-class, PowerShell 2.0, trình duyệt Wine-stack
  (Firefox ESR, Chromium portable), installer dùng TaskDialog/IFileDialog.
- **Modern hardware tối thiểu**: NVMe (stornvme), AHCI (storahci), xHCI (usbhub3), HD Audio Win7,
  ACPI 4.0 (uACPI), PCI Express bus (pcix).
- **Filesystem**: NTFS đọc/ghi ổn định, exFAT, SMB2 client tối thiểu.

### Out of scope (cố ý loại trừ)
- **Enterprise / server roles**: DirectAccess, BranchCache, AD DS, HomeGroup server-side, NPS, IIS 7.5+,
  Hyper-V root partition, SLAT/EPT, BitLocker (cần TPM + fvevol.sys).
- **Vendor GPU driver bare-metal**: NVIDIA/AMD/Intel binary KMD KHÔNG load được do ABI riêng (xem R4 §5.1).
  Chỉ hỗ trợ vGPU/virtio-gpu/VBox WDDM trong VM.
- **PatchGuard, UMS, Affinity Group, Process Reflection, WNF**: bỏ qua hoàn toàn (xem R2 §3).
- **Telemetry, Cortana, Store, WinRT**: không port.
- **Secure Boot WHQL chain**: chỉ hỗ trợ self-sign / test-sign.
- **Media Center, DVD Maker, full Media Foundation**: chỉ port mfplat skeleton để app không crash, không
  có codec pipeline đầy đủ.
- **Hyper-V guest paravirt**: storvsp/netvsc/vmbus bỏ qua.

---

## 2. Timeline tổng thể (Gantt-style text)

```
2027   Q1 ─ Q2 │ Phase 0: Stabilize NT 5.2 (foundation freeze)
                │   - ntoskrnl/mm/ARM3 zero-UNIMPLEMENTED
                │   - ntoskrnl/io/pnpmgr rebuild
                │   - freeldr/scsiport, fstub/disksup hack removal
                │   - winetest baseline + WIP debt board
                │   - drivers/bus/pcix, drivers/bus/acpi_new finalize
2027   Q3 ─ ─ ─│ Phase 1A: Vista kernel skeleton
2028               │   - bật _WIN32_WINNT=0x600 toàn ntoskrnl
                   │   - NtCreateUserProcess + NtCreateThreadEx
                   │   - Worker Factory + Tp* export thật
                   │   - Private Namespaces
                   │   - DEVPROPKEY (Io[Get|Set]DevicePropertyData)
       Q1 ─ Q2 ─ ─│ Phase 1B: ALPC + CSRSS Vista
                   │   - ALPC object type + 21 syscall
                   │   - CSRSS Vista refactor
                   │   - LSA Vista (split token tiền đề)
       Q3 ─ Q4 ─ ─│ Phase 1C: Vista user-mode foundation
                   │   - kernelbase.dll + api-set schema
                   │   - kernel32_vista (Thread Pool, Locale Ex)
                   │   - advapi32_vista (ETW v2, RegEx)
                   │   - combase Wine 10 fully wired
                   │   - bcrypt resync Wine 10
2029   Q1 ─ Q2 ─ ─│ Phase 2A: UAC + MIC
                   │   - SRM MIC enforcement
                   │   - LSA split-token + linked token
                   │   - appinfo.dll + consent.exe + Secure Desktop
                   │   - UIPI message filter trong win32k
       Q3 ─ Q4 ─ ─│ Phase 2B: Win7 shell foundation
                   │   - shell32_vista overlay (ITaskbarList3/4, IShellLibrary, IFileDialog)
                   │   - comctl32 sync Wine 10 + TaskDialog export
                   │   - uxtheme_vista mở rộng
                   │   - Explorer Win7 (Superbar + Jump Lists)
                   │   - Themes service + Aero theme parser
2030   Q1 ─ Q2 ─ ─│ Phase 2C: WASAPI + audio modern
                   │   - IAudioClient/IAudioRenderClient
                   │   - audiodg.exe
                   │   - MMCSS + Avrt
                   │   - sndvol Win7 (per-app)
       Q3 ─ Q4 ─ ─│ Phase 3A: BCD boot + winload + modern storage
                   │   - bootmgr fork freeldr + BCD hive parser
                   │   - bcdedit.exe
                   │   - winload.exe extended LOADER_PARAMETER_BLOCK
                   │   - stornvme, storahci, exFAT, SMB2 client
2031   ──────────│ Phase 3B: WDDM software path + DWM
                   │   - dxgkrnl.sys skeleton + VidMm/VidSch tối thiểu
                   │   - CDD (Canonical Display Driver)
                   │   - DXGI swapchain + d3d10/11 (Wine port)
                   │   - dwm.exe compositor + Aero Basic
2032+ ────────────│ Phase 4: WDDM real GPU (VBox/QXL/virtio-gpu) +
                   │           D2D/DWrite full + App compat lab + polish
```

Realistic milestone calendar (volunteer pace, 5 dev part-time): **5-7 năm tới Win7 desktop demo
chạy được**. WDDM tách thành sub-roadmap riêng, không block kernel/user/shell roadmap.

---

## 3. Dependency Graph (text diagram)

```
                          ┌─────────────────────────────────┐
                          │ Phase 0: NT 5.2 stabilize       │
                          │ (ARM3 MM, PnP, fstub, scsiport) │
                          └────────────┬────────────────────┘
                                       │
                  ┌────────────────────┼────────────────────┐
                  ▼                    ▼                    ▼
        [_WIN32_WINNT=0x600]    [pcix bus]          [acpi_new/uACPI]
                  │                    │                    │
                  ▼                    └────────┬───────────┘
        [NtCreateUserProcess +                  ▼
         NtCreateThreadEx]              [PnP Vista DEVPROPKEY]
                  │                             │
                  ▼                             │
        [Worker Factory]──────► [Tp* exports kernel32_vista]
                  │                             │
                  ▼                             ▼
        [ALPC kernel]    [Private Namespace]  [kernelbase + api-set]
                  │                                 │
                  ├─────────────────────────────────┤
                  ▼                                 ▼
        [CSRSS Vista]                     [combase Wine 10]
                  │                                 │
                  ▼                                 ▼
        [LSA Vista (RPC mới)]             [bcrypt Wine 10]
                  │                                 │
                  ├──────► [MIC kernel SRM]         │
                  │              │                  │
                  │              ▼                  │
                  ├──► [UAC split-token + linked]   │
                  │              │                  │
                  │              ▼                  │
                  │       [appinfo + consent.exe]   │
                  │              │                  │
                  │              ▼                  │
                  │       [UIPI win32k filter]      │
                  │                                 │
                  ▼                                 ▼
        [propsys Wine 10] ──► [shell32_vista overlay]
                                  │  (ITaskbarList3/4, IShellLibrary,
                                  │   IFileDialog, KnownFolders Win7)
                                  ▼
                          [comctl32 Wine 10 + TaskDialog export]
                                  │
                                  ▼
                          [uxtheme_vista mở rộng]
                                  │
                                  ▼
                          [Explorer Win7 Superbar + Jump Lists]
                                  │
                                  ▼
                          [Themes service + Aero msstyles]


   ───── Audio subtree (song song với UAC) ─────
   [HDAudio bus + portcls 2.x]──► [KS pin filter clean]
                                       │
                                       ▼
                              [mmdevapi IAudioClient]
                                       │
                                       ▼
                                [audiodg.exe]
                                       │
                                       ▼
                          [IAudioSessionManager2 + sndvol Win7]
                                       │
                                       ▼
                                  [MMCSS + Avrt]


   ───── BCD/Boot subtree (độc lập) ─────
   [cmlib hive parser] ─► [BCD hive reader/writer] ─► [bcdedit.exe]
                                       │
                                       ▼
                          [bootmgr fork freeldr]
                                       │
                                       ▼
              [winload.exe + extended LOADER_PARAMETER_BLOCK]


   ───── WDDM subtree (TÁCH RỜI, sub-project) ─────
   [watchdog.sys hoàn thiện] ─► [dxgkrnl.sys skeleton]
                                       │
                                       ▼
                              [VidMm + VidSch + VidPN]
                                       │
                                       ▼
                          [CDD (Canonical Display Driver)]
                                       │
                                       ▼
              [DXGI swapchain] ─► [d3d10/d3d11 Wine port]
                                       │
                                       ▼
                          [D2D/DWrite Wine port]
                                       │
                                       ▼
                              [dwm.exe compositor]
                                       │
                                       ▼
                              [Aero Basic theme]
                                       │
                                       ▼
                          [Aero Glass + thumbnail + peek]
```

**Critical chokepoints** (mất nó thì tất cả nhánh sau dừng):
1. `_WIN32_WINNT=0x600` enable ntoskrnl (gate cho mọi Vista syscall)
2. ALPC (gate cho CSRSS Vista, RPC mới, LSA Vista, audiosrv 6.x)
3. kernelbase + api-set (gate cho mọi EXE Win7+ build với manifest mới)
4. shell32_vista overlay (workaround cho shell32 forked Wine 2007-10-11)
5. dxgkrnl skeleton (gate cho mọi thứ DWM-related)

---

## 4. Team Assignment (5 vai trò)

| Vai trò | Phase 0 (2027 H1) | Phase 1 (2027 H2 - 2028) | Phase 2 (2029-2030) | Phase 3+ (2030+) |
|---------|-------------------|--------------------------|---------------------|------------------|
| **Kernel dev (Alice)** | ARM3 MM cleanup (`ntoskrnl/mm/ARM3/*` ~90 markers); fstub/disksup HACK removal (3 chỗ); freeldr scsiport (42 markers); io/pnpmgr (40 markers) | _WIN32_WINNT=0x600 enable; NtCreateUserProcess/ThreadEx; Worker Factory; Private Namespace; NtFlushProcessWriteBuffers; NtGetNextProcess/Thread; ALPC object type + 21 syscall (3-6 tháng) | MIC enforcement trong SRM; UAC token split (ntoskrnl/se); DEVPROPKEY (Io[Get|Set]DevicePropertyData); PoFx v1; KeSetCoalescableTimer honor TolerableDelay | KTM + CLFS (defer); winload.exe extended LPB; dxgkrnl integration với win32k (phối hợp với Driver dev) |
| **Win32/UI dev (Bob)** | win32k/winpos.c, window.c, defwnd.c, sysparams.c, ntstubs.c FIXME cleanup (~150 markers); GDI eng/stubs.c (89 markers); ntuser syscall stubs | **Quick win wave**: export Tp* từ rtl/threadpool.c sang kernel32_vista (3 ngày); export TaskDialog từ comctl32 (1 ngày); user32_vista DPI + Touch/Gesture stubs; advapi32_vista ETW v2; kernelbase skeleton + api-set | shell32_vista overlay (ITaskbarList3/4, IShellLibrary, IFileDialog) - port Wine; comctl32 sync Wine 10; uxtheme_vista mở rộng (BeginPanningFeedback, OpenThemeDataForDpi); Explorer Win7 Superbar + Jump Lists; Themes service | Aero theme parser (PNG animation timeline); sndvol Win7 UI; Action Center; Snipping Tool, Sticky Notes port |
| **Driver dev (Carol)** | pcix bus driver (45 + 16 + 14 + 22 markers); acpi_new uACPI OSL (44 markers); NDIS 6.x scaffolding; storage class driver refresh | DEVPROPKEY support trong driver shim; portcls 2.x WaveRT; HDAudio class driver hoàn chỉnh; ks.sys updated; stornvme + storahci skeleton | watchdog.sys hoàn thiện cho TDR; dxgkrnl.sys skeleton + DriverEntry + DXGKRNL_INTERFACE callback table; xHCI (usbhub3) | dxgkrnl VidMm/VidSch/VidPN tối thiểu; CDD; VBoxVideoWddm port làm KMD reference; QXL/virtio-gpu KMD |
| **Wine porter (Dave)** | bcrypt resync 1.9.23 -> Wine 10 (TLS modern); crypt32 resync; oleaut32 typelib FIXME; combase wire-up Wine 10 | shell32_vista port (phối hợp Bob); IFileDialog từ comdlg32/itemdlg.c Wine; propsys verify Wine 10; rpcrt4 Wine 10 wiring; advapi32 security.c (LSA Vista RPC) | dwrite.dll port từ Wine; d2d1.dll port từ Wine; mfplat skeleton (đủ không crash); wer.dll port | d3d10/d3d11/dxgi nối vào dxgkrnl thật; mf.dll codec MFT khung; gdiplus verify Wine 10 |
| **QA / Build (Eve)** | Setup winetest CI baseline; bật tất cả winetest cho oleaut32/comctl32/combase đo pass-rate; tạo "WIP debt board" script (ripgrep weekly trên UNIMPLEMENTED/FIXME/STUB); SLA: PR Vista/Win7 không tăng marker net | Regression harness so kernel32_vista/advapi32_vista exports với MS pubicAPI list; per-module winetest dashboard; rgenstat dashboard (đã có ở `sdk/tools/rgenstat/`) | App compat lab (Office 2010 Starter, .NET 4.0/4.5, PowerShell 2.0, Firefox ESR, Chrome portable, Notepad++); fuzz NtCreate* mới; ETW capture harness | App compat 70% Win7 user-mode test corpus; vendor driver test rig (VBox WDDM, virtio-gpu); release engineering Win7 ISO format |

**Single-point-of-failure cảnh báo** (R4 §5.3): Carol là dev duy nhất biết WDDM. Phase 0 cần
mentor ít nhất 1 dev khác (Alice hoặc thực tập sinh) song hành để giảm rủi ro.

---

## 5. Quick Wins Bảng (effort < 1 tuần, ROI cao)

> Liệt kê từ R2/R3 chính, mỗi item link gap cụ thể.

| # | Task | Module | Effort | ROI | Người | Link gap |
|---|------|--------|--------|-----|-------|----------|
| 1 | Export `Tp*` Thread Pool API từ `sdk/lib/rtl/threadpool.c` (đã port Wine 9.7) ra kernel32_vista.spec | kernel32 | 3 ngày | HIGH (unblock mọi app dùng .NET 4 thread pool, Office 2010, browsers modern) | Bob | R3 §5 P0 #1 |
| 2 | Thêm export `TaskDialog`, `TaskDialogIndirect` vào `dll/win32/comctl32/comctl32.spec` (code `taskdialog.c` đã có) | comctl32 | 1 ngày | HIGH (mọi installer modern, .NET WinForms) | Bob | R3 §5 P0 #4 |
| 3 | Implement `NtFlushProcessWriteBuffers` (gọi `KeFlushQueuedDpcs` broadcast tất cả CPU) | ntoskrnl/ex | 1 ngày | MEDIUM (.NET GC, lock-free C++, PPL) | Alice | R2 §2 + §4 quick wins |
| 4 | Implement `NtGetNextProcess` + `NtGetNextThread` (iterate `PspCidTable`) | ntoskrnl/ps | 2 ngày | MEDIUM (Process Hacker, ProcExp, modern toolhelp) | Alice | R2 §2 + §4 quick wins |
| 5 | Fix `KeSetCoalescableTimer` honor `TolerableDelay` thay vì bỏ qua | ntoskrnl_vista/ke.c | 2 ngày | LOW (idle power impact giảm khi laptop) | Alice | R2 §1.2 + §4 quick wins |
| 6 | `IoGetIoPriorityHint` đọc thật từ IRP extension thay vì hard-code Normal | ntoskrnl_vista/io.c | 2 ngày | LOW-MEDIUM (BITS, Defender background scan đúng priority) | Alice | R2 §1.2 + §4 |
| 7 | Implement `NtCancelIoFileEx` + `NtCancelSynchronousIoFile` | ntoskrnl/io/iomgr | 1 tuần | MEDIUM (Wine WriteFile sync cancel, libuv, Node.js) | Alice | R2 §2 |
| 8 | Implement `NtQueryLicenseValue` (đọc registry `HKLM\System\Setup`) | ntoskrnl/ex | 1 ngày | LOW (WinSAT, MS Office activation pre-check) | Alice | R2 §2 |
| 9 | Bật LFH (Low-Fragmentation Heap) mặc định trong `sdk/lib/rtl/heap.c` (code đã có) | rtl | 2 ngày | MEDIUM (giảm fragmentation cho server-class app) | Alice | R2 §3 |
| 10 | Export `RegGetValueA/W` từ advapi32_vista (Wine đã có code) | advapi32 | 2 ngày | HIGH (nhiều .NET app gọi) | Bob | R3 §4 |
| 11 | Mở rộng `advapi32_vista.spec` thêm `EventRegister/Write/Unregister/EventEnabled` (ETW user-mode) | advapi32_vista | 1 tuần | HIGH (chiến lược ETW Win7) | Bob | R3 §5 P0 #3 |
| 12 | Stub Touch/Gesture API trong `user32_vista` trả `ERROR_NOT_SUPPORTED` (RegisterTouchWindow, GetTouchInputInfo, GetGestureInfo) | user32_vista | 3 ngày | MEDIUM (app Win7 mới không crash khi check touch capability) | Bob | R3 §5 P1 #9 |
| 13 | Export `LoadIconMetric` + `LoadIconWithScaleDown` từ comctl32 (Wine đã có code) | comctl32 | 2 ngày | LOW-MEDIUM | Bob | R3 §4 |
| 14 | Bổ sung `ChangeWindowMessageFilterEx` (Win7 UIPI helper) - chỉ wrap `ChangeWindowMessageFilter` | user32_vista | 2 ngày | MEDIUM (app installer Win7 check elevation) | Bob | R3 §4 |
| 15 | Implement `QueryUnbiasedInterruptTime` (`(*KeTickCount) * KeMaximumIncrement`) | kernel32_vista | 1 ngày | MEDIUM (.NET Stopwatch, modern timers) | Bob | R3 §4 |
| 16 | Fork tail của `ntoskrnl_vista/po.c`: implement `PoRegisterPowerSettingCallback` thật (callback list + dispatch trên `PowerSetting*` notify) | ntoskrnl/po | 1 tuần | MEDIUM (Power Options UI Vista hoạt động, lid switch) | Alice | R2 §2 + §4 |
| 17 | Implement `RtlGetVersion` trả đúng 6.1 khi build flag set, để DwmIsCompositionEnabled không hard-fail | ntdll | 1 ngày | HIGH (gate cho mọi if-Vista/if-Win7 check trong app) | Alice | R5 §1 |
| 18 | Dọn `REACTOS HACK` trong `ntoskrnl/fstub/disksup.c` `xHalIoAssignDriveLetters` (3 chỗ) | ntoskrnl/fstub | 1 tuần | HIGH (block BCD adoption) | Alice | R1 §Phase 0 #3 + R5 §3 |
| 19 | Verify + bật `propsys` (Wine 10, 186 exports) trong build mặc định | propsys | 2 ngày | HIGH (gate cho shell32_vista IPropertyStore) | Dave | R3 §2 |
| 20 | Tạo skeleton `kernelbase.dll` (forward toàn bộ về kernel32 + ntdll) như bước đầu api-set | kernelbase | 1 tuần | HIGH chiến lược (mở đường app Win7+ manifest) | Bob | R3 §5 P0 #2 |

**Tổng quick wins**: 20 items, tổng effort ~6 tuần một dev. ROI lớn hơn nhiều Phase 1 đầy đủ vì giải
phóng app compat ngay.

---

## 6. Critical Path & Risk Register

| # | Risk | Mức | Mitigation |
|---|------|-----|------------|
| 1 | **WDDM showstopper**: Microsoft không công bố source dxgkrnl, vendor KMD binary KHÔNG load do ABI riêng (R4 §5.1) | **CRITICAL** | (a) Tách WDDM thành sub-project song song, không block kernel/user/shell. (b) Chấp nhận "ReactOS-specific DXGK ABI" không nhằm binary-compat với Windows. (c) Hỗ trợ VM-only (VBoxVideoWddm fork, virtio-gpu KMD viết mới). (d) Bare-metal vendor GPU = mãi mãi không hỗ trợ. (e) Aero Basic luôn fallback được dù WDDM yếu. |
| 2 | **ALPC chokepoint**: CSRSS Vista, RPC modern, LSA Vista, audiosrv đều phụ thuộc. 3-6 tháng XL effort không có shortcut (R2 §2) | **HIGH** | Bắt đầu Phase 1B ngay sau quick wins kernel. 1 dev senior chuyên trách. Tham khảo Wine `ntdll/unix/server.c` (user-side). KHÔNG nhìn Windows Research Kernel (cleanroom policy). |
| 3 | **shell32 forked Wine 2007-10-11** (R3 §2): 14 năm drift, không thể sync trực tiếp Wine 10 | **HIGH** | Tạo `shell32_vista.dll` overlay chứa toàn bộ interface Vista/Win7 (ITaskbarList3/4, IShellLibrary, IFileDialog, KnownFolderManager mở rộng) port từ Wine. shell32 chính giữ Win2000/XP behavior. Long-term: re-sync gradient theo module sub-tree (browseui, shdocvw trước, IShellFolder sau cùng). |
| 4 | **uxtheme forked**: Aero theme cần engine mới (R3 §2, R5 §1) | **MED-HIGH** | Mở rộng `uxtheme_vista.spec` (hiện 1 dòng) port BeginPanningFeedback, SetWindowThemeNonClientAttributes, OpenThemeDataForDpi từ Wine. Theme engine hiện vẽ trực tiếp GDI (không compositor) - chấp nhận cho Aero Basic, defer Glass tới Phase 3. |
| 5 | **WIP debt ngầm 23,000+ markers**: nâng version mà không vá foundation = amplify lỗi (R1) | **HIGH** | "WIP debt board" script ripgrep hằng tuần. SLA: PR Vista/Win7 không tăng marker net của module được sửa. Phase 0 mandatory 3-6 tháng zero-UNIMPLEMENTED trong ARM3 MM + pnpmgr trước khi gate Phase 1. |
| 6 | **dbgeng.c 358 stub** (R1 hotspot #1): WinDbg/CDB không thể debug | **MED** | Defer dbgeng tới Phase 4 polish. Dev dùng livekd / WinDbg trên Windows debug remote vào ReactOS qua kdcom (đã có). Không phải gate. |
| 7 | **ACPI 4.0/5.0** (uACPI port chưa nối) - laptop modern không boot (R1 Phase 0 #9) | **MED-HIGH** | Carol Phase 0 chuyên trách. uACPI upstream maintainer phải sync. Mục tiêu Phase 0 end: boot trên 3 laptop Vista/Win7-class. |
| 8 | **BCD format không stable giữa SP**: Win7 RTM vs SP1 vs Win8 khác nhau (R5 §3 Risks) | **MED** | Target Win7 SP1 RTM only. bcdedit fork không hỗ trợ Win10 BCD extensions. Test với hive lấy từ Win7 SP1 ISO. |
| 9 | **Single-point-of-failure Carol/WDDM** (R4 §5.3): Justin Miller (upstream) là dev duy nhất đẩy WDDM | **MED-HIGH** | Mentor pair: Alice + Carol cross-train trên VidMm. Document mọi DXGK design decision trong wiki. Không cho WDDM blocking. |
| 10 | **IP risk**: reverse-engineer struct nội bộ dxgkrnl, leaked source bias (R4 §5.2) | **MED** | Clean-room policy nghiêm ngặt. WDDM contributor đăng ký riêng, ký NDA negative (chưa từng xem leaked WRK/Win10 source). Pháp lý review mỗi PR WDDM. |
| 11 | **bcrypt Wine Staging 1.9.23** (R3 §2): TLS 1.2 không safe, app Win7 modern fail handshake | **HIGH** | Quick win #11 cha + bcrypt full resync Wine 10 trong Phase 1C. Verify bcryptprimitives Wine 10 đã làm reference cho symmetric. |
| 12 | **kernelbase chưa có** = api-set Win7 không hoạt động = nhiều EXE Win7 build với manifest mới không load (R3 §3) | **HIGH** | Quick win #20 skeleton + Phase 1C full. Đây là chiến lược must-have, không phải nice-to-have. |
| 13 | **Audio APO sandbox cần MIC**: audiodg đáng ra chạy protected, nhưng MIC chưa có (R5 §2 phụ thuộc) | **LOW-MED** | Implement audiodg KHÔNG isolation trước (audiodg.exe chạy SYSTEM token thường). Hardening sau khi MIC xong Phase 2A. Không block WASAPI. |
| 14 | **Media Foundation toàn bộ thiếu** (R0 §E): WMP12, Edge HTML5 video, app dùng MF codec | **MED** | Mfplat skeleton trả `E_NOTIMPL` đúng cách để app fallback DirectShow. Defer full MF tới Phase 4+. Office 2010 không cần MF, .NET WPF MediaElement cần. |
| 15 | **TPM, BitLocker, AppLocker, vault** (R0 §H): không hỗ trợ enterprise security | **LOW** | Out of scope. Document rõ trong README. Stub các API trả `STATUS_NOT_SUPPORTED`. |

---

## 7. Milestones có thể demo

| Milestone | ETA từ start | Định nghĩa "done" |
|-----------|--------------|-------------------|
| **M1: NT 5.2 stable build** | 3 tháng | Phase 0 zero-UNIMPLEMENTED trong ARM3 MM + pnpmgr; Office 2003 SP3 chạy 8h không crash; freeldr boot trên 3 laptop có ACPI 4.0; winetest baseline pass-rate >85% trên oleaut32/comctl32/combase. |
| **M2: Vista API foundation** | 12 tháng | `_WIN32_WINNT=0x600` enabled cho ntoskrnl; quick wins #1-20 done; kernelbase skeleton + 20 api-set DLL; Thread Pool app load + chạy; .NET 3.5 SP1 cài + chạy Hello World; CSRSS Vista (qua ALPC skeleton) boot tới desktop. |
| **M3: DWM software + Aero Basic** | 24 tháng | WDDM software path (CPU framebuffer) chạy; dwm.exe compositor render được desktop; theme Aero Basic apply (không glass); winver hiển thị "Windows 7" string; UAC consent prompt hoạt động end-to-end. |
| **M4: Office 2010 chạy được** | 36 tháng | Office 2010 Starter cài + chạy Word/Excel ribbon UI; PowerShell 2.0 cài + chạy script cơ bản; IFileDialog modern hoạt động trong common dialog; Jump Lists xuất hiện trên Superbar cho Notepad/MSPaint. |
| **M5: Win7 desktop experience hoàn chỉnh** | 48 tháng | Aero Glass blur-behind hoạt động (chấp nhận software shader); Libraries (Documents/Music/Pictures/Videos) hoạt động trong Explorer; Snipping Tool + Sticky Notes + WMP12 (DirectShow fallback) port xong; BCD boot + winload đọc hive Win7 SP1; WASAPI per-app slider; SMB2 client mount share Win7. |
| **M6: App compat 70% Win7 user-mode** | 60 tháng | Test corpus 100 app Win7 (.NET 4.5, browsers, Office 2010, dev tools): 70% chạy chính usecase; vendor WDDM KMD (VBoxVideoWddm fork + virtio-gpu) chạy trong VM với accelerated 2D; ReactOS Win7 ISO release candidate. |

---

## 8. Recommended Tooling / Process

### Tooling
1. **Winetest CI baseline** (Phase 0 ngay): nhúng `modules/rostests/winetests/*` vào GitHub Actions matrix
   x86 + x64. Bật full winetest cho oleaut32, comctl32, combase, propsys, rpcrt4, kernel32 - đo pass-rate
   weekly. Mục tiêu Phase 0 end >85% pass trên các DLL synced Wine 10.
2. **WIP debt board** (Eve owns): script ripgrep `(TODO|FIXME|HACK|STUB|UNIMPLEMENTED)` theo module,
   commit count weekly vào `.roadmap/wip-trend.csv` + Grafana dashboard. Block PR nếu module được sửa tăng marker net.
3. **rgenstat dashboard** tận dụng `sdk/tools/rgenstat/` đã có sẵn (R1 §Phụ lục): đếm impl/stub mỗi DLL,
   visualize trend.
4. **Wine sync helper script**: per-module `tools/winesync/<dll>.json` ghi nhận Wine version baseline +
   file map (đã có khung trong `media/doc/WINESYNC.txt`). Chạy `tools/winesync/sync.py shell32 --to 10.0`
   sinh patch + conflict report.
5. **Driver test-signing key**: tạo ReactOS Project root CA + test-sign key cho WDDM/audio driver dev,
   tích hợp vào build infra. Document workflow cho contributor.
6. **App compat test rig**: VirtualBox auto-provision script Win7 ISO + ReactOS ISO + 100-app test
   harness. Mỗi nightly chạy compat smoke (boot, launch, idle 5min, screenshot diff).
7. **Cleanroom enforcement**: pre-commit hook check author whitelist cho WDDM/dxgkrnl directory.
   Contributor WDDM sign declaration không xem WRK/leaked source.

### Process
8. **Bug triage flow**: Jira (tiếp tục dùng jira.reactos.org), label `win7-roadmap`, link tới R0-R5
   research. SLA: P0 (critical path) trả lời 48h, P1 1 tuần, P2 2 tuần.
9. **PR template** thêm checkbox: "(a) module nào, gap R nào address, (b) winetest pass-rate trước/sau,
   (c) marker count trước/sau, (d) cleanroom statement nếu WDDM".
10. **Quarterly roadmap review**: end of mỗi Q rà soát milestone, adjust timeline. Phase boundary phải
    pass criteria mới chuyển - không bỏ qua.
11. **Architecture Decision Records (ADR)**: `docs/adr/00xx-*.md` cho mỗi quyết định lớn (ALPC design,
    DXGK ABI, kernelbase scheme, shell32 overlay vs sync). Đã có template - apply nghiêm túc.
12. **Onboarding pack**: 5 báo cáo R0-R5 + roadmap này thành mandatory reading cho dev mới, kèm
    1 quick win nhỏ làm starter task (mục #3, #5, #8 trong bảng quick wins).
13. **Upstream sync cadence**: monthly sync với ReactOS main, weekly với Wine main. Document conflict
    resolution policy trong `media/doc/SYNCPOLICY.md`.
14. **Risk dashboard**: 15 risks ở §6 review monthly, owner phải báo cáo trend.

---

*Tài liệu tổng hợp R6. Không thay đổi mã. Mọi kết luận dựa trên snapshot `F:/reactos` ngày 2026-05-29
và 5 báo cáo R0-R5 trong cùng folder.*

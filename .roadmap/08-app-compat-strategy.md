# R8 - App Compatibility Strategy

> Đối tượng: team 2-5 người. Không thể implement đầy đủ Win7. Thay vào đó,
> chọn **một bộ app chuẩn** làm thước đo (yard-stick) — mỗi app thắp sáng
> một khu vực API. Khi app chạy được, ta có *bằng chứng* khu vực đó ổn.

---

## Triết lý

**"Run X to prove Y works"** thay vì cố chạy hết mọi thứ.

Hệ quả:
1. Mỗi milestone gắn với 1-2 *smoke test app*. Không pass = milestone chưa xong.
2. Ưu tiên *depth-first*: bắt 1 app chạy hết → uncover gap thật → fix → tiến app kế. Không *breadth-first* (cố stub hàng trăm API mà không có app nào chạy).
3. Compatibility shim (apphelp + shims/layer) là vũ khí giảm tải: app nào fail vì *kiểm tra version* hoặc *gọi 1 API hiếm*, dùng shim trước, viết code thật sau.
4. Chấp nhận **partial fidelity**. Office không cần ribbon đẹp; chỉ cần mở/sửa/lưu .docx không crash.
5. Dùng app làm regression test: mỗi PR lớn phải re-run smoke set.

---

## Tier A — MUST RUN (Phase 2 exit gate)

5 app đại diện, mỗi app "thắp sáng" 1 vùng:

| # | App | Vùng chứng minh |
|---|-----|----------------|
| A1 | 7-Zip 19+ | Kernel core APIs, file I/O, COM cũ, GUI cơ bản |
| A2 | Notepad++ 7.x | Scintilla, comctl32 v6, theming, file watch, codepage |
| A3 | VLC 3.x | DirectSound/WASAPI, video output (D3D9 fallback), threading |
| A4 | Office 2010 (Word + Excel) | shell32 Win7, GDI+, propsys, OLE/COM, ribbon (uiribbon), font, msi |
| A5 | Firefox ESR 52 (2017) | TLS modern (bcrypt), GDI+, audio, threadpool, JS JIT (PE+exec mem) |

### A1. 7-Zip 19.x

**Why**: Dependency thấp nhất. Pure Win32, không cần ribbon/MF/DX. Nếu 7-Zip không chạy thì kernel/user/comctl còn vỡ ở mức cơ bản.

**Dependencies & gaps**:

| Dep | Trạng thái ReactOS | Gap to close |
|-----|-------------------|--------------|
| kernel32 base I/O | OK (XP-era) | GetFileInformationByHandleEx (đã có ở `kernel32_vista`) |
| kernel32 threadpool legacy (QueueUserWorkItem) | OK | — |
| user32 + comctl32 v6 (listview, toolbar) | OK | Verify drag-drop |
| ole32 (drag-drop, IDataObject) | OK (Wine 10) | — |
| shell32 context menu, IShellFolder | OK (forked 2007, đủ cho IContextMenu cũ) | — |
| advapi32 registry | OK | — |
| GUI manifest v6 (themed controls) | uxtheme forked | Chấp nhận theme cổ điển nếu cần |

**Effort estimate**: **XS** (1 tuần). Khả năng cao chạy ngay sau khi build hiện tại fix vài stub registry.

**Smoke test (M1)**: Mở 7zFM.exe → extract `chrome-100mb.zip` → verify checksum file out. Đo thời gian extract (regression baseline).

---

### A2. Notepad++ 7.x

**Why**: Đại diện cho "modern XP-era app". Scintilla edit control, watchdog file, multi-tab UI. Không dùng ribbon. Là smoke test của **comctl32 v6 + uxtheme + codepage Unicode**.

**Dependencies & gaps**:

| Dep | Trạng thái | Gap to close |
|-----|-----------|--------------|
| kernel32 file mapping (Scintilla) | OK | — |
| comctl32 v6 (tab, status, tooltip) | Mix Wine 3.3/5.0 | Resync Wine 10 (P2) |
| user32 IME | Cũ | Đủ cho ASCII; CJK cần imm32 nâng cấp |
| msftedit / richedit 5 | Có cũ | Resync Wine 10 — Notepad++ không bắt buộc nhưng plugin có thể |
| uxtheme (themed tabs) | Forked | Chấp nhận; nếu nứt → version-lie shim "Win XP SP3" |
| FindFirstChangeNotification | OK | Verify watcher |
| Plugins (DLL load) | OK | LoadLibrary ổn |

**Effort estimate**: **S** (1-2 tuần). Có thể cần fix bug listview/tab dưới comctl32.

**Smoke test (M2)**: Mở 10 file .cpp đồng thời → C++ syntax highlight đúng → Find/Replace regex chạy → save UTF-8 BOM round-trip OK.

---

### A3. VLC 3.x

**Why**: Smoke test **audio + video output + threading nặng**. VLC tự ship codec (libavcodec) nên KHÔNG cần Media Foundation — cực kỳ phù hợp ReactOS.

**Dependencies & gaps**:

| Dep | Trạng thái | Gap to close |
|-----|-----------|--------------|
| DirectSound | `dsound_new` skeleton | Cần verify path output → WASAPI hoặc legacy waveOut |
| WASAPI (mmdevapi, audioses) | Skeleton | **P1**: implement IMMDeviceEnumerator, IAudioClient (render only) — port Wine `mmdevapi`/`winmm`. Có thể fallback `winmm waveOut`. |
| D3D9 video output | `d3d9.dll` có | Verify swap chain + StretchRect; Mesa9 backend nếu mở |
| OpenGL output | opengl32 + Mesa | OK theo R5 |
| Threadpool (libvlc dùng nhiều thread) | RTL threadpool có code | **P0**: export user-mode threadpool APIs (xem R3 §5) |
| ws2_32 (network stream) | OK | — |
| QT5 (UI VLC) | Cần `msvcr120`, `vcruntime140` | Dùng `vcredist 2013/2015` runtime |

**Effort estimate**: **M** (3-4 tuần). Audio path là blocker; nếu fallback waveOut được thì giảm xuống S.

**Smoke test (M3)**: Mở `bigbuckbunny_720p.mp4` → phát 60s không crash → seek bar di chuyển → âm thanh stereo. Fullscreen toggle.

---

### A4. Office 2010 (Word + Excel)

**Why**: Đỉnh cao của Tier A. Office 2010 = ribbon UI thật (uiribbon.dll), OOXML (zip+xml), propsys, GDI+ effect, MSI 5, OLE compound document. Nếu Office 2010 mở được .docx và in được → ReactOS đã có "Win7-class shell baseline".

Office 2010 (chứ không phải 2013/2016) vì:
- 2010 là phiên bản cuối **không** cần `apisetschema` đầy đủ (chạy được trên Vista RTM).
- 2010 không yêu cầu DirectWrite/Direct2D bắt buộc (chỉ optional cho subpixel).
- Cài đặt qua MSI cũ — KHÔNG cần Click-to-Run / AppV.

**Dependencies & gaps**:

| Dep | Trạng thái | Gap to close |
|-----|-----------|--------------|
| kernel32 Vista APIs (CreateFile2, GetTickCount64, SRW, CondVar) | Phần lớn có ở `kernel32_vista` | **P0**: export Threadpool đầy đủ |
| kernelbase.dll | KHÔNG TỒN TẠI | **P0**: tạo khung forward sang kernel32, ntdll |
| shell32: IFileDialog, IShellLibrary, IShellItem2 | Thiếu | **P1**: tạo `shell32_vista.dll` overlay (port Wine 10) |
| shell32: ITaskbarList3 (jump list) | Thiếu | **P1**: ditto (jump list có thể no-op shim) |
| comdlg32: ItemDialog (FileOpen/Save Vista+) | Thiếu | Port Wine `comdlg32/itemdlg.c` |
| comctl32 v6.10: TaskDialog | Code có, **chưa export** | **P0 quick win**: thêm export trong `.spec` |
| GDI+ (effects, ImageAttributes Win7) | Wine 4 hoặc 10 (verify) | Resync Wine 10 chắc chắn |
| propsys.dll | **Wine 10 OK** | — |
| ole32 / oleaut32 | Wine 10 / 4.18 | OK |
| msi 5.0 | Wine 9.8 | OK — Office 2010 installer test |
| uiribbon.dll | **KHÔNG CÓ** | **P1**: viết shim stub trả lỗi → Office fallback "compat menu". Hoặc port Wine `uiribbon` nếu có. |
| ms-help, OneNote shared | Bundled với Office (mso*.dll) | OK — chỉ cần load PE |
| Font: Segoe UI, Calibri, Cambria | Có thể bundle hoặc nhúng từ Office | Verify ClearType qua gdi32 |
| usp10.dll (uniscribe) | Có | Verify Arabic/CJK nếu cần |
| crypt32 / wintrust (Office signature check) | Wine 10 / Wine 4 | Crypt32 cần update CNG path |
| Mso compatibility shim (Office tự check OS version) | apphelp engine có | **versionlie shim** Win7 SP1 cho `winword.exe`, `excel.exe` |

**Effort estimate**: **L** (8-12 tuần). Phụ thuộc P0 (threadpool, kernelbase) + P1 (shell32_vista, comdlg32 itemdlg). Không cần ribbon đẹp — chấp nhận classic toolbar fallback.

**Smoke test (M4 + M5)**:
- M4: WINWORD.EXE khởi động, mở `lorem-10page.docx`, edit text, save → DOC sửa giữ format heading + bullet.
- M5: EXCEL.EXE mở `sales-chart.xlsx`, chart bar render đúng, thêm formula `=SUM(A1:A10)` cho ra kết quả, save.

---

### A5. Firefox ESR 52 (2017)

**Why**: Đại diện browser **modern crypto + threadpool nặng + JIT**. ESR 52 là bản cuối hỗ trợ XP/Vista chính thức → tương thích cao với ReactOS hiện tại. TLS 1.2 + SHA-2 ép `bcrypt` phải lên Wine 10.

**Dependencies & gaps**:

| Dep | Trạng thái | Gap to close |
|-----|-----------|--------------|
| kernel32 threadpool | code có | **P0** export user-mode |
| bcrypt + bcryptprimitives | bcrypt cũ (Wine 1.9.23) — primitives Wine 10 | **P2**: resync bcrypt Wine 10. Firefox tự dùng NSS nhưng `bcryptPrimitives` cho RNG |
| advapi32 EventLog ETW | thiếu | Stub OK (Firefox không bắt buộc) |
| wininet / winhttp | Wine 10 OK | — |
| ws2_32 (IPv6, getaddrinfo) | OK | Verify dual-stack |
| d3d9 / d2d (HW accel) | d3d9 có, d2d thiếu | Firefox tự fallback Basic Layers |
| uxtheme | forked | OK; classic theme |
| dwmapi (vsync) | Wine 8.14 | OK |
| msvcr/vcruntime (VS2013 runtime) | Có (msvcrt + redist) | Verify |
| Sandbox (job objects, integrity level) | MIC thiếu | Firefox tắt sandbox được (`MOZ_DISABLE_CONTENT_SANDBOX=1`) |
| Mark-of-the-Web (zone identifier) | NTFS ADS có | OK |

**Effort estimate**: **M** (4-6 tuần) sau khi A4 đã xong (chung dependency).

**Smoke test (M6)**: Mở `https://www.wikipedia.org` (TLS 1.2). Render HTML đúng. Search box gõ chữ. Đóng tab không crash. Memory ổn định 5 phút.

---

## Tier B — SHOULD RUN (Phase 3, sau Phase 2 đã xong A1-A5)

Ngắn gọn — mỗi app + lý do + blocker chính:

| App | Khu vực mới mở ra | Blocker chính |
|-----|-------------------|---------------|
| **Steam** (offline mode) | Steam overlay (hook DX9/11), CEF (Chromium) | Cần WDDM-lite hoặc d3d11 trên Mesa Gallium (xem R4) |
| **Visual Studio Code** | Electron 4-9 (v8 + Node), webview2 | Node native modules: `fs`, `child_process`, named pipe |
| **Photoshop CS6** | GDI+ heavy, color management (mscms), 16-bit/channel | mscms Vista+ rewrite; GPU compositing tắt được |
| **.NET 4.x apps (paint.net, LINQPad)** | CLR 4 host, fusion, NGEN | CoreCLR/desktop .NET 4 — ReactOS đã từng có dotnet support; cần verify runtime mới |
| **Discord** (old build 2019) | Electron + WebRTC | giống VS Code + WASAPI nâng cao |
| **Git for Windows** (CLI) | MSYS2 emulation | Có shim "msys2" trong appcompat hiện có |
| **Sumatra PDF** | Direct2D, DirectWrite | Cần port d2d/dwrite (P1) |
| **WinRAR** | Như 7-Zip | Sau A1 là OK |
| **OBS Studio 27** (cũ) | DXGI capture, WASAPI loopback | DXGI thiếu — block |

**Strategy**: Chỉ chọn 2-3 trong list này làm "stretch goal" Phase 3.

---

## Tier C — SKIP (với lý do)

| App / family | Lý do skip |
|--------------|-----------|
| **UWP / Microsoft Store apps** | Cần WinRT (Windows.Foundation, ABI metadata), AppContainer, packaging. Effort = tái viết SDK Windows 8+. Không đáng. |
| **DirectX 12 games** (Cyberpunk, Forza...) | Cần WDDM 2.x + DXGI 1.4+ + d3d12 + DXR. Vượt mục tiêu Win7. |
| **Modern Office 365 / Office 2019+** | Click-to-Run + AppV + apisetschema đầy đủ + DComp. Office 2010 đủ chứng minh. |
| **Azure CLI / Microsoft Teams** | OAuth2 modern (web account manager), TLS 1.3, WebView2 (Edge Chromium). |
| **Windows Defender / Microsoft AV** | ELAM, PPL (Protected Process Light), kernel mini-filter. Quá xâm lấn. |
| **WSL / Hyper-V** | picohyperv driver, vmswitch — chip-level. |
| **Visual Studio 2019+ IDE** | .NET 4.7.2+, WPF nặng, NuGet, Roslyn. Dùng VS Code thay thế. |
| **Adobe Creative Cloud 2019+** | Adobe Application Manager + CEF + GPU compositing. |
| **Chrome 90+ / Edge** | Sandbox cứng, mojo IPC, swiftshader. Firefox ESR 52 đủ chứng minh browser. |
| **HoloLens / Surface Pen / Cortana** | Sensor, ink, ML stack. |

---

## Milestone smoke tests (M1 → M6)

| M | Gate | App test | Đo lường |
|---|------|----------|---------|
| **M1 — Foundation OK** | Build + boot stable | **7-Zip** extract 100 MB zip | Hoàn tất < 30 s, checksum khớp |
| **M2 — Modern XP-era app** | comctl32 v6 healthy | **Notepad++** + 10 file C++ | Syntax highlight + Find/Replace regex |
| **M3 — Media stack baseline** | WASAPI hoặc waveOut + d3d9 video | **VLC** phát mp4 720p | 60 s không drop frame, fullscreen toggle |
| **M4 — Office baseline** | shell32_vista + kernelbase + GDI+ Wine 10 | **Word 2010** mở docx | Edit + save round-trip |
| **M5 — Office advanced** | propsys + msi5 + uiribbon shim | **Excel 2010** mở xlsx có chart | Chart render + formula recompute |
| **M6 — Modern browser** | bcrypt Wine 10 + threadpool full | **Firefox ESR 52** HTTPS | wikipedia.org load, scroll, search |

**Phase 2 exit gate** = M1 ∧ M2 ∧ M3 ∧ M4 ∧ M5 ∧ M6.

Pass criteria mỗi smoke test:
- Khởi động < 60s, không hộp thoại "missing DLL"
- Hoàn tất kịch bản test mà không crash
- Memory stable 5 phút (idle)
- Đo regression: compare run-time vs run trước (cảnh báo > 30% slowdown)

---

## Compatibility shim strategy

ReactOS đã có **apphelp.dll** + shim engine (`dll/appcompat/apphelp/shimeng.c`) và shim provider modules (`dll/appcompat/shims/genral`, `shims/layer`). Tận dụng triệt để:

### Shim sẵn có để dùng
- `layer/versionlie.c` — gắn nhãn OS = Win7 SP1 cho EXE cụ thể. Office 2010 / Firefox / Steam đều check version. **Quick win**: thêm entry SDB cho `winword.exe`, `excel.exe`, `steam.exe`, `firefox.exe`.
- `layer/forcedxsetupsuccess.c` — bypass DirectX installer check.
- `layer/ignoreloadlibrary.c` — skip DLL không tồn tại không gây fail.
- `genral/themes.c` — disable themes cho app không hợp uxtheme cũ.
- `genral/msys2.c` — đã có shim cho MSYS2 (Git Bash).

### Shim cần viết mới
| Shim | Mục đích | Target app |
|------|---------|-----------|
| `IgnoreApiSet` | Skip load `api-ms-win-*.dll`, forward về DLL chính | App Win7+ |
| `FakeUIRibbon` | Trả `E_NOTIMPL` từ `UIRibbonCreateFramework` → app fallback classic | Office 2010, WordPad, MSPaint Win7 |
| `RedirectKernelbase` | Bắt LoadLibrary(`kernelbase.dll`) → trả handle kernel32 cho tới khi kernelbase thật xong | Bridging shim |
| `FakeJumpList` | No-op `ITaskbarList3` để app không crash khi register jump list | Office, Firefox |
| `ForceClassicCommonDialog` | Ép `GetOpenFileName` trả về dialog cũ thay vì `IFileOpenDialog` mới | App nào fail itemdlg |
| `MfPlatformStub` | Return failure rõ ràng cho `MFStartup` → app fallback DirectShow | Photoshop, Premiere preview |

### SDB database
- Quản lý qua `sysmain.sdb` (Win7 dùng). ReactOS có sdbwrite/sdbread.
- Tạo `reactos-shim-db.xml` source-controlled → build thành `.sdb` → ship trong AppCompat folder.
- Mỗi entry: `{exe-name, file-size, checksum-MZ, shim-list, version-lie-target}`.

---

## App Compat Lab Setup

### Test images
Tạo trước 3 VM snapshot làm baseline:
- **bench-clean.qcow2** — ReactOS vừa cài, chưa app.
- **bench-runtime.qcow2** — đã cài VC++ 2008/2010/2013/2015 redist, .NET 4.0.
- **bench-fullshim.qcow2** — runtime + custom shim DB.

### Automation harness
```
F:/reactos/.compat-lab/
├── apps/                  # MSI/EXE installer (gitignored, fetched from blob storage)
├── scripts/
│   ├── run-smoke.ps1     # parameterized: -App 7zip -Scenario extract-100mb
│   ├── collect-logs.ps1  # gather DbgPrint, app crashes, memory
│   └── compare-baseline.ps1
├── corpora/               # input files: zip, docx, xlsx, mp4, pdf
├── expected/              # checksum + screenshot baseline per scenario
└── reports/               # JSON results per CI run
```

Harness chạy headless qua KVM/QEMU monitor + screenshot diff (pixel tolerance 5%).

### CI tích hợp
- Mỗi PR đánh nhãn `app-critical`: run M1+M2 (nhanh, < 10 phút).
- Nightly: chạy đủ M1-M6 trên revision HEAD; report regression.
- Weekly: run Tier B (3 app trong list) → publish trend chart.

### Bug tracker categories
Label trên Jira/GitHub Issues:
- `app:7zip`, `app:notepad++`, `app:vlc`, `app:office`, `app:firefox` — gắn tag app trigger bug.
- `area:kernel32`, `area:shell32`, `area:gdi32`, `area:audio`, `area:directx` — vùng API gốc.
- `priority:P0-app-blocker` — chặn smoke test ở milestone hiện tại.
- `shim-candidate` — có thể giải bằng shim trước khi fix thật.
- `wine-port` — fix sẵn ở Wine; chỉ cần resync.

### Reporting cadence
- Bảng dashboard nội bộ: 1 dòng / app / milestone, cell màu (xanh PASS / vàng PARTIAL / đỏ FAIL).
- Họp tuần: review thay đổi cell màu + bug mới gắn `app:*`.

---

## Tổng kết

- **5 app Tier A** là *contract* cho Phase 2. Mỗi app gắn 1 milestone, 1 smoke test repeatable.
- Tận dụng **apphelp + shim engine có sẵn** + thêm SDB tùy chỉnh để tránh viết code thật cho mọi gap.
- Tier B chỉ là stretch goal — đừng phân tán nỗ lực trước khi Tier A xong.
- Tier C là *hard NO* — không tốn thời gian thử.
- Lab automation phải dựng song song code work, không để cuối kỳ.

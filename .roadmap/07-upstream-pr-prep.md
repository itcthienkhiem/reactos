# R7 - Upstream PR Preparation (tách commit `aafe459ddd9`)

> Mục tiêu: gom-tách commit "update fix" `aafe459ddd9` (gộp 5 module Win7 work) thành các commit per-module theo style `reactos/reactos` để PR upstream.
> Ngày: 2026-05-29. Branch nguồn: `main` (clone từ itcthienkhiem/reactos), commit gốc: `aafe459ddd9`.

> ⚠️ **Lưu ý quan trọng**: Doc này được biên soạn trong môi trường KHÔNG có quyền chạy `git`. Các phần `git show --stat`, `git log upstream/master` cần được người vận hành chạy lại để điền số liệu thực; nội dung subject/body/file/script bên dưới được dựng từ phạm vi mô tả nhiệm vụ + đọc trực tiếp cây nguồn hiện tại (post-commit state). Trước khi PR phải verify lại file list bằng `git show aafe459ddd9 --name-only`.

---

## 1. Tổng quan commit gốc `aafe459ddd9`

### 1.1 Lệnh cần chạy để lấy stats thực

```bash
git -C F:/reactos show aafe459ddd9 --stat | tee .roadmap/_aafe459-stat.txt
git -C F:/reactos show aafe459ddd9 --name-only --pretty=format: | sort -u | tee .roadmap/_aafe459-files.txt
git -C F:/reactos show aafe459ddd9 --format="%H%n%an <%ae>%n%ad%n%s%n%n%b" --no-patch
```

### 1.2 Phạm vi đã biết (theo mô tả nhiệm vụ + đối chiếu cây nguồn)

| Nhóm logic | Đường dẫn được động chạm (xác nhận tồn tại trên cây) |
|---|---|
| PROPSYS | `dll/win32/propsys/propvar.c`, `dll/win32/propsys/propsys.spec`, `sdk/include/psdk/propvarutil.h` |
| SHELL32 | `dll/win32/shell32/CShellItem.cpp`, `dll/win32/shell32/CShellItem.h`, `dll/win32/shell32/wine/shell32_main.c`, `dll/win32/shell32/wine/shell32_main.h`, `dll/win32/shell32/shell32.spec`, `sdk/include/psdk/shellapi.h`, `sdk/include/psdk/shlobj.h`, `sdk/include/psdk/shobjidl.idl` |
| BCRYPT | `dll/win32/bcrypt/bcrypt_main.c`, `sdk/include/psdk/bcrypt.h` |
| DWMAPI | `dll/win32/dwmapi/dwmapi_main.c` |
| ⚠️ Rác cần loại | `.claude/settings.local.json` (settings IDE, KHÔNG được vào PR upstream) |

### 1.3 Cảnh báo file rác trong commit gốc

`.claude/settings.local.json` xuất hiện trong commit gốc theo ghi chú của task. File này thuộc cấu hình IDE cá nhân, không thể đẩy ra upstream. Khi tách commit BẮT BUỘC loại file này (`git restore --staged` / không add lại). Nếu cần xoá hẳn khỏi history fork trước khi PR, dùng `git filter-repo` (xem mục 7).

---

## 2. Quan sát style commit ReactOS upstream

### 2.1 Lệnh cần chạy để xác nhận empirically

```bash
git -C F:/reactos log --oneline upstream/master -100
git -C F:/reactos log upstream/master -50 --format="%s%n%b%n---"
git -C F:/reactos log upstream/master -- dll/win32/shell32 | head -200
```

### 2.2 Patterns đã biết của ReactOS upstream (rút từ CODING_STYLE.md + lịch sử dự án)

1. **Subject = `[MODULE]` hoặc `[MODULE1][MODULE2]` prefix viết HOA**, theo sau là 1 câu mô tả ngắn dạng imperative (Add / Fix / Implement / Update / Sync). Ví dụ thực tế thường thấy: `[SHELL32] Implement ...`, `[NTOS:MM] Fix ...`, `[BCRYPT] Sync with Wine x.y`. Khi đồng thời chạm DLL + SDK header thì ghép `[SHELL32][SDK]`. Subject ≤ 72 ký tự (mục tiêu 60).
2. **Khi sync code Wine** thường có hậu tố `Sync with Wine-X.Y` hoặc `Import from Wine staging` + dòng "CORE-#####" nếu có Jira ticket. Code Wine không bị reformat theo style ReactOS (theo CODING_STYLE.md mục đầu).
3. **Khi chỉ format**: prefix `[FORMATTING]` đứng RIÊNG, không trộn với thay đổi logic (CODING_STYLE.md đã quy định rõ).
4. **Body**: dòng trống sau subject, sau đó 2-5 dòng giải thích WHY (Vista/Win7 dependency, CORE-ID, ứng dụng nào cần). Liệt kê file dưới dạng prose hoặc bullet. Không bắt buộc `Signed-off-by` (ReactOS không enforce DCO như Linux), nhưng nhiều committer vẫn kèm `CORE-NNNNN` nếu có Jira ticket.
5. **Không có dòng `Co-Authored-By: Claude …`** trong upstream — phải xoá trước khi PR.

---

## 3. Commit 1 — `[PROPSYS][SDK] Add propvar conversion helpers for Vista PROPVARIANT API`

### 3.1 Subject (≤60)
```
[PROPSYS][SDK] Add propvar conversion helpers
```

### 3.2 Body
```
[PROPSYS][SDK] Add propvar conversion helpers

Wire up the PropVariant conversion/utility surface needed by the
Vista IPropertyStore-based shell stack (IShellItem2, IFileDialog).
Extend propvar.c implementations and expose missing prototypes plus
PROPVAR_CHANGE_FLAGS / PVCU_* enums in propvarutil.h so consumers
in shell32/comdlg32 link cleanly.

Required by [SHELL32] Vista IShellItem work that lands in the next
commit; CORE-### (fill in if there is a Jira ticket).
```

### 3.3 Files
- `dll/win32/propsys/propvar.c`
- `dll/win32/propsys/propsys.spec` (nếu có export mới)
- `sdk/include/psdk/propvarutil.h`

### 3.4 Test plan note
- Build `propsys` + winetest `propsys_winetest`: `ninja propsys_apitest` (hoặc test set Wine tương ứng trong `modules/rostests/winetests/propsys/`).
- Smoke test InitPropVariantFrom* và PropVariantChangeType với loại được thêm.

### 3.5 Cherry-pick script
```bash
# tại fork root
git fetch upstream master
git checkout -b upstream/pr-propsys-vista upstream/master
git checkout aafe459ddd9 -- \
    dll/win32/propsys/propvar.c \
    dll/win32/propsys/propsys.spec \
    sdk/include/psdk/propvarutil.h
# verify không kéo theo file ngoài scope
git status --short
git diff --cached --stat
git commit -F .roadmap/_msg-propsys.txt   # nội dung body 3.2
```

---

## 4. Commit 2 — `[SHELL32][SDK] Implement Vista IShellItem surface`

### 4.1 Subject (≤60)
```
[SHELL32][SDK] Implement Vista IShellItem surface
```

### 4.2 Body
```
[SHELL32][SDK] Implement Vista IShellItem surface

Bring CShellItem / SHCreateItemFrom* in line with the Vista IShellItem
contract used by IFileDialog, IExplorerBrowser and shell extensions
ported from Win7. Adds the FMTID_Storage PROPERTYKEY shims missing from
propkey.h, fleshes out wine/shell32_main glue and exports the new
entry points via shell32.spec.

SDK side updates shellapi.h / shlobj.h / shobjidl.idl prototypes and
COM interface declarations consumers depend on; depends on the
[PROPSYS][SDK] commit landing first. CORE-### (fill if applicable).
```

### 4.3 Files
- `dll/win32/shell32/CShellItem.cpp`
- `dll/win32/shell32/CShellItem.h`
- `dll/win32/shell32/wine/shell32_main.c`
- `dll/win32/shell32/wine/shell32_main.h`
- `dll/win32/shell32/shell32.spec`
- `sdk/include/psdk/shellapi.h`
- `sdk/include/psdk/shlobj.h`
- `sdk/include/psdk/shobjidl.idl`

### 4.4 Test plan note
- Build `shell32` + winetest `shell32_apitest` (`modules/rostests/apitests/shell32/`), đặc biệt nhánh IShellItem / SHCreateItemFromParsingName.
- Smoke: chạy `explorer` mở Save As dialog (yêu cầu IFileDialog), drop file vào CShellItem → check GetDisplayName.
- Nếu chạm IDL: rebuild `widl` consumers, đảm bảo midl-pe / proxy stub không lỗi.

### 4.5 Cherry-pick script
```bash
git checkout -b upstream/pr-shell32-vista upstream/master
git checkout aafe459ddd9 -- \
    dll/win32/shell32/CShellItem.cpp \
    dll/win32/shell32/CShellItem.h \
    dll/win32/shell32/wine/shell32_main.c \
    dll/win32/shell32/wine/shell32_main.h \
    dll/win32/shell32/shell32.spec \
    sdk/include/psdk/shellapi.h \
    sdk/include/psdk/shlobj.h \
    sdk/include/psdk/shobjidl.idl
git status --short
git diff --cached --stat
git commit -F .roadmap/_msg-shell32.txt
```

---

## 5. Commit 3 — `[BCRYPT][SDK] Extend CNG primitives for Vista TLS stack`

### 5.1 Subject (≤60)
```
[BCRYPT][SDK] Extend CNG primitives for Vista
```

### 5.2 Body
```
[BCRYPT][SDK] Extend CNG primitives for Vista

Round out bcrypt_main.c so the BCryptOpenAlgorithmProvider / Hash /
SymKey path matches the Vista CNG contract consumed by schannel,
ncrypt and the credential UI shim. Adds missing algorithm IDs and
property constants to bcrypt.h so headers no longer drift from the
mbedTLS-backed implementation.

Building block for the TLS 1.2 / NCrypt work tracked in CORE-###.
Wine-sync: keep upstream Wine layout untouched per CODING_STYLE.md
(no reformatting of synchronized files).
```

### 5.3 Files
- `dll/win32/bcrypt/bcrypt_main.c`
- `sdk/include/psdk/bcrypt.h`

### 5.4 Test plan note
- winetest `bcrypt_winetest` trong `modules/rostests/winetests/bcrypt/` — đo baseline trước/sau.
- Smoke: `BCryptHashData` SHA-256/SHA-384, `BCryptGenerateSymmetricKey` AES-128-GCM.
- Cross-check không regress KSecDD (driver/ksecdd) vì share IOCTL.

### 5.5 Cherry-pick script
```bash
git checkout -b upstream/pr-bcrypt-vista upstream/master
git checkout aafe459ddd9 -- \
    dll/win32/bcrypt/bcrypt_main.c \
    sdk/include/psdk/bcrypt.h
git status --short
git diff --cached --stat
git commit -F .roadmap/_msg-bcrypt.txt
```

---

## 6. Commit 4 — `[DWMAPI] Report composition state per Vista contract`

### 6.1 Subject (≤60)
```
[DWMAPI] Report composition state per Vista contract
```

### 6.2 Body
```
[DWMAPI] Report composition state per Vista contract

Make DwmIsCompositionEnabled / DwmExtendFrameIntoClientArea behave
like Vista+: report composition based on the running OS version
(RtlGetVersion) instead of returning a hard-coded value, so apps
gating Aero glass paths take the correct branch.

Pure DLL change, no SDK header churn. Test-only impact is the dwmapi
winetest under modules/rostests/winetests/dwmapi/.
```

### 6.3 Files
- `dll/win32/dwmapi/dwmapi_main.c`

### 6.4 Test plan note
- winetest `dwmapi_winetest`.
- Smoke: Aero-aware app (ví dụ explorer skin Vista) gọi `DwmIsCompositionEnabled` → đảm bảo trả `*enabled` đúng theo `RtlGetVersion`.

### 6.5 Cherry-pick script
```bash
git checkout -b upstream/pr-dwmapi-vista upstream/master
git checkout aafe459ddd9 -- dll/win32/dwmapi/dwmapi_main.c
git status --short
git diff --cached --stat
git commit -F .roadmap/_msg-dwmapi.txt
```

---

## 7. Combined script (4 PR branch riêng từ commit gốc)

```bash
#!/usr/bin/env bash
set -euo pipefail

REPO=F:/reactos
SRC=aafe459ddd9
cd "$REPO"

git fetch upstream master

# helper: tạo branch sạch từ upstream/master, lấy file từ SRC, commit
make_pr_branch () {
    local branch="$1" ; shift
    local msgfile="$1" ; shift
    git checkout upstream/master
    git checkout -b "$branch"
    git checkout "$SRC" -- "$@"
    # double-check không có file ngoài scope
    git diff --cached --name-only
    git commit -F "$msgfile"
}

# 1. PROPSYS + SDK
make_pr_branch upstream/pr-propsys-vista .roadmap/_msg-propsys.txt \
    dll/win32/propsys/propvar.c \
    dll/win32/propsys/propsys.spec \
    sdk/include/psdk/propvarutil.h

# 2. SHELL32 + SDK
make_pr_branch upstream/pr-shell32-vista .roadmap/_msg-shell32.txt \
    dll/win32/shell32/CShellItem.cpp \
    dll/win32/shell32/CShellItem.h \
    dll/win32/shell32/wine/shell32_main.c \
    dll/win32/shell32/wine/shell32_main.h \
    dll/win32/shell32/shell32.spec \
    sdk/include/psdk/shellapi.h \
    sdk/include/psdk/shlobj.h \
    sdk/include/psdk/shobjidl.idl

# 3. BCRYPT + SDK
make_pr_branch upstream/pr-bcrypt-vista .roadmap/_msg-bcrypt.txt \
    dll/win32/bcrypt/bcrypt_main.c \
    sdk/include/psdk/bcrypt.h

# 4. DWMAPI
make_pr_branch upstream/pr-dwmapi-vista .roadmap/_msg-dwmapi.txt \
    dll/win32/dwmapi/dwmapi_main.c

# Sanity: liệt kê 4 branch và HEAD subject
for b in upstream/pr-propsys-vista upstream/pr-shell32-vista \
         upstream/pr-bcrypt-vista  upstream/pr-dwmapi-vista ; do
    echo "== $b =="
    git log -1 --oneline "$b"
done
```

### 7.1 File message kèm theo (tạo trong `.roadmap/` rồi xoá sau khi commit)

Tạo 4 file `.roadmap/_msg-propsys.txt`, `_msg-shell32.txt`, `_msg-bcrypt.txt`, `_msg-dwmapi.txt` với body lấy từ mục 3.2 / 4.2 / 5.2 / 6.2. Không thêm dòng `Co-Authored-By: Claude …`.

### 7.2 Verify trước khi push

```bash
# 1. Diff giữa branch PR vs commit gốc đúng phạm vi
git diff upstream/pr-propsys-vista..aafe459ddd9 -- $(git diff --name-only upstream/master upstream/pr-propsys-vista)
# 2. Không có .claude/, .vscode/, .idea/, *.user, build/ trong commit
git show upstream/pr-propsys-vista --name-only | grep -E '^\.(claude|vscode|idea)/|/build/' && echo "STOP" || echo "OK"
# Lặp cho 3 branch còn lại.
```

---

## 8. Lưu ý PR upstream

1. **`.claude/settings.local.json` trong history `aafe459ddd9`**: chỉ "không add khi tách" là đủ cho 4 PR mới. Nếu muốn xoá hoàn toàn khỏi history fork (mọi branch), dùng:
   ```bash
   pip install git-filter-repo
   git -C F:/reactos filter-repo --path .claude --invert-paths
   ```
   Cảnh báo: `filter-repo` rewrite SHA → mất luôn `aafe459ddd9`. Chỉ chạy sau khi đã extract xong 4 branch trên và backup remote `origin`.

2. **Mỗi PR cần kèm winetest pass**: ReactOS yêu cầu PR đụng DLL có winetest phải báo cáo `winetest_old vs winetest_new` (xem template PR upstream). Lệnh chạy local:
   ```bash
   ninja -C build bcrypt_winetest propsys_apitest shell32_apitest dwmapi_winetest
   build/modules/rostests/winetests/bcrypt/bcrypt_winetest.exe
   # ... v.v.
   ```
   Ghi log pass/fail vào body PR (mục "Testing done").

3. **Coding style ReactOS** (rút từ `F:/reactos/CODING_STYLE.md`):
   - **Indent 4 SPACES, không tab**.
   - Brace luôn ở dòng riêng (Allman style).
   - Line width ≤ 100 ký tự.
   - `NULL`/`TRUE`/`FALSE`, không phải `0`/`true`/`false` cho Win32 boolean.
   - Header `#pragma once`, không guard define.
   - File header SPDX-style `PROJECT/LICENSE/PURPOSE/COPYRIGHT`.
   - **Ngoại lệ**: code đồng bộ với Wine (propsys/propvar.c, bcrypt_main.c, dwmapi_main.c, shell32/wine/*) **KHÔNG được reformat** — giữ style Wine để tiện wine-sync sau này. `media/doc/WINESYNC.txt` track mapping.
   - Commit chỉ format → tag `[FORMATTING]`, không trộn logic.

4. **Author identity**: trước khi tách, set lại user.email/user.name của 4 branch về định danh muốn xuất hiện trên upstream:
   ```bash
   git -C F:/reactos config user.email "<email>"
   git -C F:/reactos config user.name "<full name>"
   ```
   (Tránh dùng email noreply của Anthropic / Claude trong commit upstream.)

5. **PR description template**: mỗi PR nên có Summary (1-2 câu), Why (link CORE-###/MS docs), Changes (bullet file), Testing (winetest before/after).

6. **Thứ tự merge khuyến nghị**: PROPSYS → SHELL32 (depends propsys) → BCRYPT (độc lập) → DWMAPI (độc lập). Có thể mở song song nhưng PR SHELL32 phải base trên branch sau khi PROPSYS đã merge, nếu không CI build sẽ fail link.

---

## 9. TODO cho người vận hành trước khi PR

- [ ] Chạy `git show aafe459ddd9 --stat` và dán vào mục 1.1 để xác nhận file list không khác với 4 nhóm ở 1.2 (nếu khác, cập nhật script ở mục 7).
- [ ] Chạy `git log upstream/master -50 --format="%s"` và soi 2-3 commit gần đây cùng module để chỉnh subject prefix cho match (ví dụ `[SHELL32]` vs `[SHELL32_APITEST]`).
- [ ] Tạo 4 file `.roadmap/_msg-*.txt` rồi chạy script ở 7.
- [ ] Chạy winetest, dán kết quả vào PR body.
- [ ] Loại `.claude/` khỏi staging trước mỗi `git commit` (đã được lọc bởi `git checkout aafe459ddd9 -- <whitelist>` nên chỉ cảnh giác file mới phát sinh).
- [ ] Reset `user.email`/`user.name` về identity upstream.
- [ ] Đẩy branch + mở PR riêng cho từng module.

/*************************************************************************/
/*  dir_access_windows.cpp                                               */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2019 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2019 Godot Engine contributors (cf. AUTHORS.md).   */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "core/error_macros.h"
#include "core/string_utils.inl"
#if defined(WINDOWS_ENABLED)

#include "dir_access_windows.h"

#include "core/os/memory.h"

#include "core/ustring.h"

#include <cstdio>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <QStorageInfo>

/*

[03:57] <reduz> yessopie, so i don't havemak to rely on unicows
[03:58] <yessopie> reduz- yeah, all of the functions fail, and then you can call GetLastError () which will return 120
[03:58] <drumstick> CategoryApl, hehe, what? :)
[03:59] <CategoryApl> didn't Verona lead to some trouble
[03:59] <yessopie> 120 = ERROR_CALL_NOT_IMPLEMENTED
[03:59] <yessopie> (you can use that constant if you include winerr.h)
[03:59] <CategoryApl> well answer with winning a compo

[04:02] <yessopie> if ( SetCurrentDirectoryW ( L"." ) == FALSE && GetLastError () == ERROR_CALL_NOT_IMPLEMENTED ) { use
ANSI }
*/

struct DirAccessWindowsPrivate {

    HANDLE h; // handle for findfirstfile
    WIN32_FIND_DATA f;
    WIN32_FIND_DATAW fu; // unicode version
};

// CreateFolderAsync

Error DirAccessWindows::list_dir_begin() {

    _cisdir = false;
    _cishidden = false;

    list_dir_end();
    p->h = FindFirstFileExW(StringUtils::from_utf8(current_dir + "\\*").toStdWString().c_str(), FindExInfoStandard, &p->fu,
            FindExSearchNameMatch, nullptr, 0);

    return (p->h == INVALID_HANDLE_VALUE) ? ERR_CANT_OPEN : OK;
}
bool DirAccessWindows::has_next() const {
    return (p->h != INVALID_HANDLE_VALUE);
}

String DirAccessWindows::get_next() {

    if (p->h == INVALID_HANDLE_VALUE)
        return String();

    _cisdir = (p->fu.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
    _cishidden = (p->fu.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN);
    ;
    String name = StringUtils::to_utf8(StringUtils::from_wchar(p->fu.cFileName));

    if (FindNextFileW(p->h, &p->fu) == 0) {

        FindClose(p->h);
        p->h = INVALID_HANDLE_VALUE;
    }

    return name;
}

bool DirAccessWindows::current_is_dir() const {

    return _cisdir;
}

bool DirAccessWindows::current_is_hidden() const {

    return _cishidden;
}

void DirAccessWindows::list_dir_end() {

    if (p->h != INVALID_HANDLE_VALUE) {

        FindClose(p->h);
        p->h = INVALID_HANDLE_VALUE;
    }
}
int DirAccessWindows::get_drive_count() {

    return drive_count;
}
String DirAccessWindows::get_drive(int p_drive) {

    if (p_drive < 0 || p_drive >= drive_count) return String();

    return String(&drives[p_drive],1) + ":";
}
#include <QDebug>
Error DirAccessWindows::change_dir(StringView _dir)
{
    String p_dir = fix_path(_dir);

    QString real_current_dir_name = QDir::currentPath();
    QString tgt_dir = StringUtils::from_utf8(p_dir);
    QFileInfo fz(StringUtils::from_utf8(current_dir)+ tgt_dir);

    QDir cur_dir(StringUtils::from_utf8(current_dir));
    bool worked = cur_dir.cd(tgt_dir);

    const String base(_get_root_path());
    if (!base.empty()) {

        real_current_dir_name = cur_dir.path();
        const String new_dir(StringUtils::to_utf8(real_current_dir_name));
        if (!StringUtils::begins_with(new_dir,base)) {
            worked = false;
        }
    }

    if (worked) {

        real_current_dir_name = cur_dir.absolutePath();
        current_dir = StringUtils::to_utf8(real_current_dir_name);
    }

    return worked ? OK : ERR_INVALID_PARAMETER;
}

Error DirAccessWindows::make_dir(StringView _dir) {

    GLOBAL_LOCK_FUNCTION

    String p_dir = fix_path(_dir);
    if (PathUtils::is_rel_path(p_dir)) p_dir = PathUtils::plus_file(current_dir,p_dir);

    QDir dir(StringUtils::from_utf8(p_dir));
    if (!dir.exists()) {
        return dir.mkdir(".") ? OK : ERR_CANT_CREATE;
    }
    return ERR_ALREADY_EXISTS;
}

String DirAccessWindows::get_current_dir() {

    String base = _get_root_path();
    if (base.empty()) return current_dir;

    String bd = StringUtils::replace_first(current_dir,base, "");
    if (StringUtils::begins_with(bd,"/")) return _get_root_string() + StringUtils::substr(bd,1, bd.length());
    return _get_root_string() + bd;
}

String DirAccessWindows::get_current_dir_without_drive() {

    String dir = get_current_dir();

    if (_get_root_string().empty()) {
        auto p = current_dir.find(":");
        if (p != String::npos) {
            dir = dir.substr(p + 1);
        }
    }

    return dir;
}

bool DirAccessWindows::file_exists(StringView _file) {

    GLOBAL_LOCK_FUNCTION
    String p_file(_file);
    if (!PathUtils::is_abs_path(p_file))
        p_file = PathUtils::plus_file(get_current_dir(),p_file);

    p_file = fix_path(p_file);

    // StringUtils::replace(p_file,"/","\\");
    QFileInfo fi(StringUtils::from_utf8(p_file));
    return fi.exists() && fi.isFile();
}

bool DirAccessWindows::dir_exists(StringView _dir) {

    GLOBAL_LOCK_FUNCTION

    String p_dir(_dir);
    if (PathUtils::is_rel_path(p_dir))
        p_dir = PathUtils::plus_file(get_current_dir(),p_dir);

    p_dir = fix_path(p_dir);

    QFileInfo fi(StringUtils::from_utf8(p_dir));
    return fi.exists() && fi.isDir();
}

Error DirAccessWindows::rename(StringView _path, StringView _new_path) {
    String p_path(_path);
    String p_new_path(_new_path);
    if (PathUtils::is_rel_path(p_path))
        p_path = PathUtils::plus_file(get_current_dir(), p_path);

    p_path = fix_path(p_path);

    if (PathUtils::is_rel_path(p_new_path))
        p_new_path = PathUtils::plus_file(get_current_dir(), p_new_path);

    p_new_path = fix_path(p_new_path);
    return QFile::rename(StringUtils::from_utf8(p_path), StringUtils::from_utf8(p_new_path)) ? OK : FAILED;
}

Error DirAccessWindows::remove(StringView _path) {

    String p_path(_path);

    if (PathUtils::is_rel_path(p_path)) p_path = PathUtils::plus_file(get_current_dir(),p_path);

    p_path = fix_path(p_path);

    printf("erasing %.*s\n", int(p_path.length()),p_path.data());

    QString str_path(StringUtils::from_utf8(p_path));
    QFileInfo fi(str_path);
    if (!fi.exists())
        return FAILED;
    if (fi.isDir()) {
        return QDir().rmdir(str_path) ? OK : FAILED;
    }
    return QFile::remove(str_path) ? OK : FAILED;
}
/*

FileType DirAccessWindows::get_file_type(const String& p_file) const {


    wchar_t real_current_dir_name[2048];
    GetCurrentDirectoryW(2048,real_current_dir_name);
    String prev_dir=real_current_dir_name;

    bool worked SetCurrentDirectoryW(current_dir.c_str());

    DWORD attr;
    if (worked) {

        WIN32_FILE_ATTRIBUTE_DATA    fileInfo;
        attr = GetFileAttributesExW(p_file.c_str(), GetFileExInfoStandard, &fileInfo);

    }

    SetCurrentDirectoryW(prev_dir.c_str());

    if (!worked)
        return FILE_TYPE_NONE;


    return (attr&FILE_ATTRIBUTE_DIRECTORY)?FILE_TYPE_
}
*/
uint64_t DirAccessWindows::get_space_left() {

    uint64_t bytes = 0;
    if (!GetDiskFreeSpaceEx(nullptr, (PULARGE_INTEGER)&bytes, nullptr, nullptr)) return 0;

    // this is either 0 or a value in bytes.
    return bytes;
}

String DirAccessWindows::get_filesystem_type() const {
    String path = fix_path(const_cast<DirAccessWindows *>(this)->get_current_dir());
    int unit_end = StringUtils::find(path,":");
    ERR_FAIL_COND_V(unit_end == -1, {});
    String unit = String(StringUtils::substr(path,0, unit_end + 1)) + "\\";
    QStorageInfo info(StringUtils::from_utf8(path));
    return StringUtils::to_utf8(info.fileSystemType());
}

DirAccessWindows::DirAccessWindows() : current_dir(".")  {

    p = memnew(DirAccessWindowsPrivate);
    p->h = INVALID_HANDLE_VALUE;


    DWORD mask = GetLogicalDrives();

    for (int i = 0; i < MAX_DRIVES; i++) {

        if (mask & (1 << i)) { // DRIVE EXISTS

            drives[drive_count] = 'A' + i;
            drive_count++;
        }
    }

    DirAccessWindows::change_dir(".");
}

DirAccessWindows::~DirAccessWindows() {

    memdelete(p);
}

#endif // windows DirAccess support

import json
import os
import csv
import sys
sys.path.append('/Applications/Understand.app/Contents/MacOS/Python')
import understand
print('understand version:', understand.version())

# 要记录的参数
column = ["RelativePath", "FunctionName", "dep", "dep_count", "dexp", "dexp_count"]

# input and output method
inputs= [
    "canonicalize_file_name",
    "catgets",
    "confstr",
    "ctermid",
    "cuserid",
    "dgettext",
    "dngettext",
    "fgetc",
    "fgetc_unlocked",
    "fgets",
    "fgets_unlocked",
    "fpathconf",
    "fread",
    "fread_unlocked",
    "fscanf",
    "getc",
    "getchar",
    "getchar_unlocked",
    "getc_unlocked",
    "get_current_dir_name",
    "getcwd",
    "getdelim",
    "getdelim",
    "__getdelim",
    "getdents",
    "getenv",
    "gethostbyaddr",
    "gethostbyname",
    "gethostbyname2",
    "gethostent",
    "gethostid",
    "getline",
    "getlogin",
    "getlogin_r",
    "getmsg",
    "getopt",
    "_getopt_internal",
    "getopt_long",
    "getopt_long_only",
    "getpass",
    "getpmsg",
    "gets",
    "gettext",
    "getw",
    "getwd",
    "ngettext",
    "pathconf",
    "pread",
    "pread64",
    "ptsname",
    "ptsname_r",
    "read",
    "readdir",
    "readlink",
    "readv",
    "realpath",
    "recv",
    "recv_from",
    "recvmesg",
    "scanf",
    "__secure_getenv",
    "signal",
    "sysconf",
    "ttyname",
    "ttyname_r",
    "vfscanf",
    "vscanf"
]
outputs = [
    "dprintf",
    "fprintf",
    "fputc",
    "fputchar_unlocked",
    "fputc_unlocked",
    "fputs",
    "fputs_unlocked",
    "fwrite",
    "fwrite_unlocked",
    "perror",
    "printf",
    "psignal",
    "putc",
    "putchar",
    "putc_unlocked",
    "putenv",
    "putmsg",
    "putpmsg",
    "puts",
    "putw",
    "pwrite",
    "pwrite64",
    "send",
    "sendmsg",
    "sendto",
    "setenv",
    "sethostid",
    "setlogin",
    "ungetc",
    "vdprintf",
    "vfprintf",
    "vsyslog",
    "write",
    "writev"
]

# part1 处理proftpd
proftpd_und_path = './data/proftpd-1.3.5b.und'
proftpd_output_path = './data/proftpd.csv'

db = understand.open(proftpd_und_path)

# 读入数据库中的函数
funcs = db.ents(" ".join(["function", "~unknown", "~unresolved"]))

# data记录所有参数
data = []
for func in funcs:
    # 相对路径
    if func.parent():
        func_relpath = func.parent().relname()
    else:
        func_relpath = ""
    # 函数名
    func_name = func.name()

    dep, dexp = [], []
    for call in func.ents("call"):
        if call.name() in inputs:
            dep.append(call.name())
        if call.name() in outputs:
            dexp.append(call.name())

    row = [func_relpath, func_name, dep, len(dep), dexp, len(dexp)]
    data.append(row)

with open(proftpd_output_path, 'w') as output:
    writer = csv.writer(output)
    writer.writerow(column)
    writer.writerows(data)



# part2 处理wu-ftpd
wuftpd_und_path = './data/wu-ftpd-2.6.2.und'
wuftpd_output_path = './data/wuftpd.csv'

db = understand.open(wuftpd_und_path)

# 读入数据库中的函数
funcs = db.ents(" ".join(["function", "~unknown", "~unresolved"]))

# data记录所有参数
data = []
for func in funcs:
    # 相对路径
    if func.parent():
        func_relpath = func.parent().relname()
    else:
        func_relpath = ""

    func_name = func.name()

    dep, dexp = [], []
    for call in func.ents("call"):
        if call.name() in inputs:
            dep.append(call.name())
        if call.name() in outputs:
            dexp.append(call.name())

    row = [func_relpath, func_name, dep, len(dep), dexp, len(dexp)]
    data.append(row)

with open(wuftpd_output_path, 'w') as output:
    writer = csv.writer(output)
    writer.writerow(column)
    writer.writerows(data)

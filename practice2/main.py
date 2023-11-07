import os
import sys
import csv
import wget
import collections
import bisect
from functools import cmp_to_key
sys.path.append('/Applications/Understand.app/Contents/MacOS/Python')
import understand

print('understand version:', understand.version())


def get_patches(url_root, file_root):
    for i in range(1, 54):
        file_name = 'bash42-' + str(i).zfill(3)
        patch_url = url_root + file_name
        file_path = file_root + file_name
        if os.path.exists(file_path):
            print(f'file {file_path} already existed!')
        else:
            print(f'downloading...{file_name}')
            wget.download(patch_url, file_root)
            print('\n')


def process_functions(und_path):
    '''
    处理und数据库，返回一个字典结构:
    {"file.c":[
        {
            "name": "func1",
            "start": 250,
            "end": 450,
        },
        {
            "name": "func2",
            "start": 555,
            "end": 666,
        },
    ]
    "file2.h":[
        ......
    ]
    }
    '''
    # 载入und文件, 从und文件读函数
    db = understand.open(und_path)
    funcs = db.ents("function")
    data = {}
    for func in funcs:
        # 函数名、函数有多少行，函数所在的文件
        func_name = func.name()
        func_countline = func.metric(['CountLine'])['CountLine']
        if func.parent():
            func_file = func.parent().relname()
        else:
            func_file = func.relname()
        # 函数起始行
        func_define = func.refs('Definein')

        for define in func_define:
            line_start = define.line() - 1
            line_end = line_start + func_countline
            func_file = func_file[func_file.find('/')+1:]
            data.setdefault(func_file, []).append({
                "name": func_name,
                "start": line_start,
                "end": line_end,
            })


    return data

def process_patches(file_root):
    file_names = sorted(os.listdir(file_root))
    file_paths = [
        os.path.join(file_root, file_name) for file_name in file_names
    ]
    data = []
    for file_path in file_paths:
        # 拆分每一个patch，每个拆分成以一行为单位的列表
        lines = open(file_path).read().splitlines()
        data.extend(process_patch(lines))
    return data


def process_patch(lines):
    # 先是一个***打头的行，标识patch打在哪个文件，然后后面跟几个*** xx, xxxx ****, 标识在此文件中修改的行号start end
    result = []
    for line in lines:
        if line.startswith('*** '):
            if not line.endswith(' ****'):
                file = process_file(line)
            else:
                start, end = process_start_and_end(line)
                result.append({
                    "file": file,
                    "start": start,
                    "end": end,
                })
    return result

def process_start_and_end(line):
    # *** 12,34 ****
    start, end = line[4:-5].split(",")
    return int(start), int(end)

def process_file(line):
    # *** ../bash-4.2-patched/..../subst.c	2011-01-02 16:12:51.000000000 -0500
    parts = line.split()
    file = parts[1]
    # ..../subst.c
    return "/".join(file.split("/")[2:])

@cmp_to_key
def cmp(f1, f2):
    f1_s, f1_e = f1["start"], f1["end"]
    f2_s, f2_e = f2["start"], f2["end"]
    return (f1_s-f2_s or f1_e-f2_e)

def main():

    file_root = './patches/'
    url_root = 'https://ftp.gnu.org/gnu/bash/bash-4.2-patches/'
    get_patches(url_root, file_root)

    und_path = './bash-4.21.und'
    output_path = './output.csv'

    functions = process_functions(und_path)
    patches = process_patches(file_root)

    '''
    处理函数后
    {"file.c":[
        {
            "name": "func1",
            "start": 250,
            "end": 450,
        },
        {
            "name": "func2",
            "start": 555,
            "end": 666,
        },
    ]
    "file2.h":[
        ......
    ]}
    处理补丁后
    [
    {
        "file": xx,
        "start": xx,
        "end": xx,
    },
    {
    
    },
    ...
    ]

    '''

    counter = collections.Counter()
    for patch in patches:
        file = patch["file"]
        # 根据file拿函数的list funcs
        funcs = functions.get(file)
        if funcs == None:
            continue
        funcs = sorted(funcs, key=cmp)

        starts = [func["start"] for func in funcs]
        ends = [func["end"] for func in funcs]
        start, end = patch["start"], patch["end"]

        ranges=[]
        for a, b in zip(starts, ends):
            ranges.append(a)
            ranges.append(b)
        
        insex_start = bisect.bisect(ranges, start)
        index_end = bisect.bisect(ranges, end)
        insex_start = insex_start // 2 # 因为start end在一起
        index_end = index_end // 2 + index_end % 2

        for func in funcs:
            funcname = func["name"]
            counter[funcname] += 0

        for func in funcs[insex_start:index_end]:
            funcname = func["name"]
            counter[funcname] += 1

    with open(output_path, 'w') as output:
        writer = csv.writer(output)
        writer.writerow(['Function', 'Count'])
        for funcname, count in counter.items():
            writer.writerow([funcname, count])

if __name__ == "__main__":
    main()

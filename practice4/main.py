import os
import sys
import csv
import collections
from math import log2
# sys.path.append('/Applications/Understand.app/Contents/MacOS/Python')
# import understand
print('understand version:', understand.version())


def GetHalstedBaseMetrics(lexemes):
    operators = collections.Counter()
    operands = collections.Counter()
    for lexeme in lexemes:
        token, text = lexeme.token(), lexeme.text()
        if token in ["Operator", "Keyword", "Punctuation"]:
            if text not in [")", "]", "}"]:
                operators[text]+=1
        elif token in ["Identifier", "Literal", "String"]:
            operands[text]+=1
    
    mu1 = len(operators)
    mu2 = len(operands)
    n1 = sum(operators.values())
    n2 = sum(operands.values())

    return mu1, mu2, n1, n2



if __name__ == "__main__":

    # 数据库路径和输出csv路径
    und_path = './gcc-3.4.0.und'
    output_path = './output.csv'
    db = understand.open(und_path)
    # 读入函数
    funcs = db.ents(" ".join(["function", "~unknown", "~unresolved"]))
    # print(funcs)
    # 要记录的参数
    column = ["RelativePath", "FunctionName", "N", "V", "D", "E", "L", "T"]
    # data记录所有参数
    data = []

    for func in funcs:
        # 函数名
        func_name = func.name()
        # 相对路径
        if func.parent():
            func_relpath = func.parent().relname()
        else:
            func_relpath = ""
        
        mu1, mu2, n1, n2 = GetHalstedBaseMetrics(func.lexer().lexemes())
        func_N = n1+n2
        func_V = func_N*log2(mu1+mu2)

        res = func.metric(["CountInput", "CountOutput"])
        count_intput = res["CountInput"]
        count_output = res["CountOutput"]
        mu2_asterisk = count_intput+count_output
        v_asterisk = (2+mu2_asterisk)*log2(2+mu2_asterisk)

        func_L = v_asterisk/func_V
        func_D = 1/func_L
        func_E = func_V/func_L
        func_T = func_E/18
        row = [func_relpath, func_name, func_N, func_V, func_D, func_E, func_L, func_T]
        data.append(row)
        print(row)
    
    with open(output_path, 'w') as output:
        writer = csv.writer(output)
        writer.writerow(column)
        writer.writerows(data)



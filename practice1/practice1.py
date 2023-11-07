import os
import sys
import csv
sys.path.append('/Applications/Understand.app/Contents/MacOS/Python')
import understand
print('understand version:', understand.version())

# 数据库路径和输出csv路径
und_path = './gnuit-4.9.5.und'
output_path = './output.csv'
db = understand.open(und_path)

# 要记录的参数
metrics = ["CountLineCode", "CountPath", "Cyclomatic", "MaxNesting", "Knots", "CountInput", "CountOutput"]
column = ["FunctionName", "RelativePath"] + metrics

# 读入数据库中的函数
funcs = db.ents("function")

# data记录所有参数
data = []
for func in funcs:
  # 获取函数的名字、相对路径、各种指标
  func_name = func.name()
  func_relpath = func.relname()
  func_metircs=func.metric(metrics)

  # 整合一行数据  
  row = [func_name, func_relpath] + list(func_metircs.values()) 
  data.append(row)
  # print(row)

# 写入到csv文件中
with open(output_path, 'w') as output:
  writer = csv.writer(output)
  writer.writerow(column)
  writer.writerows(data)
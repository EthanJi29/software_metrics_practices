options(repos = structure(c(CRAN = "https://cran.rstudio.com/")))

# install.packages("readr")
# install.packages("psych")
# install.packages("e1071")
# install.packages("tidyverse")
# install.packages("dplyr")
# install.packages("caret")

# library(caret)
# model_list <- getModelInfo()
# names(model_list)


# 获取xalan-2.4.csv数据集列名
data <- read.csv("xalan-2.4.csv", header = TRUE, sep = ",")
variables <- colnames(data)
print(variables)
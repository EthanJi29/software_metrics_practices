library(readr)
library(psych)
library(e1071)
library(tidyverse)
library(dplyr)
library(caret)

Sys.setenv(LANGUAGE="en")
# 下载数据集
destfile <- "xalan-2.4.csv"
if(file.exists(destfile)){
  print("Dataset already exists")
} else {
  url <- "https://zenodo.org/record/268436/files/xalan-2.4.csv"
  download.file(url, destfile)
}

# 读取数据集
data <- read.csv(destfile, header = TRUE, sep = ",")
# 选择需要分析的变量
variables <- c("wmc", "dit", "noc", "cbo", "rfc", "lcom")

# 任务a
desc_stats <- tibble(
    variables = variables,
    min = sapply(data[variables], min),
    `1st Qu` = sapply(data[variables], quantile, 0.25),
    median = sapply(data[variables], median),
    mean = sapply(data[variables], mean),
    `3rd Qu` = sapply(data[variables], quantile, 0.75),
    max = sapply(data[variables], max),
    skewness = sapply(data[variables], skewness),
    kurtosis = sapply(data[variables], kurtosis),
    
)
write.csv(desc_stats, "result_a.csv", row.names = FALSE)
print("mission a completed")

# 任务b
correrlation <- tibble(
  variables = variables,
  `spearman with "bug"` = sapply(data[variables], function(x) cor.test(x, data[,"bug"], method = "spearman", exact=FALSE)$estimate),
  `pearson with "bug"` = sapply(data[variables], function(x) cor.test(x, data[,"bug"], method = "pearson", exact=FALSE)$estimate),
)
write.csv(correrlation, "result_b.csv", row.names = FALSE)
print("mission b completed")

# 任务c
# 利用Naïve Bayes和logistic回归等10种机器学习方法建立多变量的缺陷预测模型，不需要进行特征选择
train_control <- trainControl(method = "cv", number = 10)

# Naïve Bayes
model_nb <- train(bug ~ ., data = data, method = "nb", trControl = train_control)
# logistic回归
model_glm <- train(bug ~ ., data = data, method = "glm", trControl = train_control)
# 支持向量机
model_svm <- train(bug ~ ., data = data, method = "svmRadial", trControl = train_control)
# K近邻
model_knn <- train(bug ~ ., data = data, method = "knn", trControl = train_control)
# 决策树
model_rpart <- train(bug ~ ., data = data, method = "rpart", trControl = train_control)
# 随机森林
model_rf <- train(bug ~ ., data = data, method = "rf", trControl = train_control)
# Adaboost
model_adaboost <- train(bug ~ ., data = data, method = "adaboost", trControl = train_control)
# Bagging
model_bag <- train(bug ~ ., data = data, method = "treebag", trControl = train_control)
# 神经网络
model_nnet <- train(bug ~ ., data = data, method = "nnet", trControl = train_control)
# 梯度提升
model_gbm <- train(bug ~ ., data = data, method = "gbm", trControl = train_control)

print("mission c completed")


# 任务d
# 利用10x10交叉验证评估模型的预测性能，使用AUC和CE值作为评估指标
model_list <- list(model_nb, model_glm, model_svm, model_knn, model_rpart, model_rf, model_adaboost, model_bag, model_nnet, model_gbm)
model_names <- c("Naïve Bayes", "logistic回归", "支持向量机", "K近邻", "决策树", "随机森林", "Adaboost", "Bagging", "神经网络", "梯度提升")
model_performance <- tibble(
  model = model_names,
  AUC = sapply(model_list, function(x) x$results$ROC[1]),
  CE = sapply(model_list, function(x) x$results$ROC[2])
)
write.csv(model_performance, "result_c.csv", row.names = FALSE)
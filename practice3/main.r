library(readr)
library(psych)
library(e1071)
library(tidyverse)
library(dplyr)
library(caret)
library(klaR)
library(kernlab)
library(gbm)
library(mlr3)
library(mlr3verse)
library(pheatmap)
library(kknn)
library(glmnet)
library(Hmisc)
devtools::install_github("b0rxa/scmamp")
library(scmamp)
library(Rgraphviz)


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
  `spearman p-value` = sapply(data[variables], function(x) cor.test(x, data[,"bug"], method = "spearman", exact=FALSE)$p.value),
  `pearson p-value` = sapply(data[variables], function(x) cor.test(x, data[,"bug"], method = "pearson", exact=FALSE)$p.value),
)
write.csv(correrlation, "result_b.csv", row.names = FALSE)
print("mission b completed")


# 任务c
# 利用Naïve Bayes和logistic回归等10种机器学习方法建立多变量的缺陷预测模型，不需要进行特征选择
# 首先对数据集的bug列进行二值化
data$bug <- ifelse(data$bug > 0, 1, 0)
# naive bayes是分类模型，要把bug列转换为factor类型
data$bug <- as.factor(data$bug)
data <- subset(data, select=c("wmc", "dit", "noc", "cbo", "rfc", "lcom", "bug"))

tasks = as_task_classif(data, target = "bug", id = "func")
models = c(
    "classif.naive_bayes", 
    "classif.log_reg",
    "classif.glmnet",
    "classif.multinom",
    "classif.qda",
    "classif.rpart",
    "classif.svm",
    "classif.lda",
    "classif.kknn",
    "classif.featureless")
learners = lrns(models, predict_type = "prob", predict_sets = c("test"))
print("mission c completed")

# 任务d
# 10x10交叉验证
resampling = rsmp("repeated_cv", repeats=10, folds = 10)
design = benchmark_grid(tasks, learners, resampling)
# 运行
bmr = benchmark(design)
# print(bmr)
# 得到bmr的结果中的auc和ce
keys <- list(msr("classif.auc", id="auc"), msr("classif.ce", id="ce"))
scores <- bmr$score(keys)
scores <- subset(scores, select=c("learner_id", "auc", "ce"))
auc <- data.frame()
ce <- data.frame()
for(i in 0:9){
  for(j in 1:100){
    auc[j, i+1] <- scores[i*100+j, 2]
    ce[j, i+1] <- scores[i*100+j, 3]
  }
}
# 输出结果
cols <- c("naive_bayes", "log_reg", "glmnet", "multinom", "qda", "rpart", "svm", "lda", "kknn", "featureless")
colnames(auc) <- cols
colnames(ce) <- cols
write.csv(auc, "result_d_auc.csv", row.names = FALSE)
write.csv(ce, "result_d_ce.csv", row.names = FALSE)
print("mission d completed")


# 任务e
# plotcd
plotCD(results.matrix=auc, alpha=0.01)
plotCD(results.matrix=ce, alpha=0.01)
print("mission e completed")

# 任务f
# 通过drawAlgorithmGraph函数，绘制模型的algorithm图
rank = auc[, -2]
pv.matrix <- friedmanAlignedRanksPost(data=rank, control=NULL)
pv.adj <- adjustBergmannHommel(pv.matrix)
r.means <- colMeans(rankMatrix(rank))
drawAlgorithmGraph(pvalue.matrix=pv.adj, mean.value=r.means, alpha=0.05, font.size=8, node.width=4, node.height=2)

# 任务g
pheatmap(as.matrix(auc))
pheatmap(as.matrix(ce))
print("mission g completed")
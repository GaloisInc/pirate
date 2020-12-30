args <- commandArgs(trailingOnly=TRUE)
if (length(args) != 2) {
    stop("Usage - process.py [send results] [receive results]", call.=FALSE)
}
send <- read.table(args[1], sep='\t', numerals='no.loss')
receive <- read.table(args[2], sep='\t', numerals='no.loss')
delta <- receive[,3] - send[,3]
cat('mean', mean(delta), '\n')
cat('stdev', sd(delta), '\n')
cat('90% percentile', quantile(delta, c(0.9))[1], '\n')

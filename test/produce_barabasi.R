library(igraph)
 
for (num.vertices in seq(2000000, 10000000, 1000000)) {
  g <- barabasi.game(num.vertices)
  write.graph(g,  sprintf("%s-%d.%s", "barabasi", num.vertices, "txt"))
}

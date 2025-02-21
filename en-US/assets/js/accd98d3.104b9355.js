"use strict";(self.webpackChunkdocusaurus=self.webpackChunkdocusaurus||[]).push([[17414],{28453:(e,t,i)=>{i.d(t,{R:()=>l,x:()=>h});var n=i(96540);const r={},a=n.createContext(r);function l(e){const t=n.useContext(a);return n.useMemo((function(){return"function"==typeof e?e(t):{...t,...e}}),[t,e])}function h(e){let t;return t=e.disableParentContext?"function"==typeof e.components?e.components(r):e.components||r:l(e.components),n.createElement(a.Provider,{value:t},e.children)}},36979:(e,t,i)=>{i.r(t),i.d(t,{assets:()=>s,contentTitle:()=>l,default:()=>d,frontMatter:()=>a,metadata:()=>h,toc:()=>o});var n=i(74848),r=i(28453);const a={},l="TuGraph Built-in Algorithm Description",h={id:"developer-manual/interface/olap/algorithms",title:"TuGraph Built-in Algorithm Description",description:"This document mainly introduces the TuGraph built-in algorithm program in detail, the community version of 6 algorithms can refer to the basic algorithm newspaper",source:"@site/versions/version-4.0.1/en-US/source/5.developer-manual/6.interface/2.olap/6.algorithms.md",sourceDirName:"5.developer-manual/6.interface/2.olap",slug:"/developer-manual/interface/olap/algorithms",permalink:"/tugraph-db/en-US/en/4.0.1/developer-manual/interface/olap/algorithms",draft:!1,unlisted:!1,tags:[],version:"current",sidebarPosition:6,frontMatter:{},sidebar:"tutorialSidebar",previous:{title:"Python Olap API",permalink:"/tugraph-db/en-US/en/4.0.1/developer-manual/interface/olap/python-api"},next:{title:"TuGraph Stored Procedure Guide",permalink:"/tugraph-db/en-US/en/4.0.1/developer-manual/interface/procedure/procedure"}},s={},o=[{value:"1.Introduction",id:"1introduction",level:2},{value:"1.1.Basic algorithms:",id:"11basic-algorithms",level:3},{value:"1.2.Extended algorithms:",id:"12extended-algorithms",level:3},{value:"2.Basic algorithms",id:"2basic-algorithms",level:2},{value:"2.1.Breadth-First Search",id:"21breadth-first-search",level:3},{value:"2.2.Pagerank",id:"22pagerank",level:3},{value:"2.3.Single-Source Shortest Path",id:"23single-source-shortest-path",level:3},{value:"2.4.Weakly Connected Components",id:"24weakly-connected-components",level:3},{value:"2.5.Local Clustering Coefficient",id:"25local-clustering-coefficient",level:3},{value:"2.6.Label Propagation Algorithm",id:"26label-propagation-algorithm",level:3},{value:"3.Extended algorithms",id:"3extended-algorithms",level:2},{value:"3.1.All-Pair Shortest Path",id:"31all-pair-shortest-path",level:3},{value:"3.2.Betweenness Centrality",id:"32betweenness-centrality",level:3},{value:"3.3.Belief Propagation",id:"33belief-propagation",level:3},{value:"3.4.Closeness Centrality",id:"34closeness-centrality",level:3},{value:"3.5.Common Neighborhood",id:"35common-neighborhood",level:3},{value:"3.6.Degree Correlation",id:"36degree-correlation",level:3},{value:"3.7.Dimension Estimation",id:"37dimension-estimation",level:3},{value:"3.8.EgoNet",id:"38egonet",level:3},{value:"3.9.Hyperlink-Induced Topic Search",id:"39hyperlink-induced-topic-search",level:3},{value:"3.10.Jaccard Index",id:"310jaccard-index",level:3},{value:"3.11.K-core",id:"311k-core",level:3},{value:"3.12.Louvain",id:"312louvain",level:3},{value:"3.13.Multiple-source Shortest Paths",id:"313multiple-source-shortest-paths",level:3},{value:"3.14.Personalized PageRank",id:"314personalized-pagerank",level:3},{value:"3.15.Strongly Connected Components",id:"315strongly-connected-components",level:3},{value:"3.16.Speaker-listener Label Propagation Algorithm",id:"316speaker-listener-label-propagation-algorithm",level:3},{value:"3.17.Single-Pair Shortest Path",id:"317single-pair-shortest-path",level:3},{value:"3.18.Triangle Counting",id:"318triangle-counting",level:3},{value:"3.19.Trustrank",id:"319trustrank",level:3},{value:"3.20.Weighted Label Propagation Algorithm",id:"320weighted-label-propagation-algorithm",level:3},{value:"3.21.Weighted Pagerank Algorithm",id:"321weighted-pagerank-algorithm",level:3},{value:"3.22.Maximal independent set",id:"322maximal-independent-set",level:3},{value:"3.23.Sybil Rank",id:"323sybil-rank",level:3},{value:"3.24.Subgraph Isomorphism",id:"324subgraph-isomorphism",level:3},{value:"3.25.Motif",id:"325motif",level:3},{value:"3.26.Kcliques",id:"326kcliques",level:3},{value:"3.27.Ktruss",id:"327ktruss",level:3},{value:"3.28.Leiden",id:"328leiden",level:3}];function c(e){const t={a:"a",blockquote:"blockquote",em:"em",h1:"h1",h2:"h2",h3:"h3",header:"header",p:"p",table:"table",tbody:"tbody",td:"td",th:"th",thead:"thead",tr:"tr",...(0,r.R)(),...e.components};return(0,n.jsxs)(n.Fragment,{children:[(0,n.jsx)(t.header,{children:(0,n.jsx)(t.h1,{id:"tugraph-built-in-algorithm-description",children:"TuGraph Built-in Algorithm Description"})}),"\n",(0,n.jsxs)(t.blockquote,{children:["\n",(0,n.jsx)(t.p,{children:"This document mainly introduces the TuGraph built-in algorithm program in detail, the community version of 6 algorithms can refer to the basic algorithm newspaper"}),"\n"]}),"\n",(0,n.jsx)(t.h2,{id:"1introduction",children:"1.Introduction"}),"\n",(0,n.jsx)(t.p,{children:"TuGraph currently contains the following 6 basic algorithms and 28 extended algorithms, a total of 34 graph algorithms:"}),"\n",(0,n.jsx)(t.h3,{id:"11basic-algorithms",children:"1.1.Basic algorithms:"}),"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n",(0,n.jsxs)(t.table,{children:[(0,n.jsx)(t.thead,{children:(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.th,{style:{textAlign:"center"},children:"Algorithm name"}),(0,n.jsx)(t.th,{style:{textAlign:"center"},children:"The program name"})]})}),(0,n.jsxs)(t.tbody,{children:[(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Breadth-First Search"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"bfs"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Pagerank"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"pagerank"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Single-Source Shortest Path"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"sssp"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Weakly Connected Components"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"wcc"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Local Clustering Coefficient"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"lcc"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Label Propagation Algorithm"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"lpa"})]})]})]}),"\n",(0,n.jsx)(t.h3,{id:"12extended-algorithms",children:"1.2.Extended algorithms:"}),"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n",(0,n.jsxs)(t.table,{children:[(0,n.jsx)(t.thead,{children:(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.th,{style:{textAlign:"center"},children:"Algorithm name"}),(0,n.jsx)(t.th,{style:{textAlign:"center"},children:"The program name"})]})}),(0,n.jsxs)(t.tbody,{children:[(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"All-Pair Shortest Path"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"apsp"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Betweenness Centrality"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"bc"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Belief Propagation"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"bp"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Closeness Centrality"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"clce"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Common Neighborhood"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"cn"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Degree Correlation"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"dc"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Dimension Estimation"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"de"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"EgoNet"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"en"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Hyperlink-Induced Topic Search"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"hits"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Jaccard Index"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"ji"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"K-core"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"kcore"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Louvain"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"louvain"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Multiple-source Shortest Paths"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"mssp"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Personalized PageRank"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"ppr"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Strongly Connected Components"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"scc"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Speaker-listener Label Propagation Algorithm"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"slpa"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Single-Pair Shortest Path"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"spsp"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Triangle Counting"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"triangle"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Trustrank"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"trustrank"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Weighted Label Propagation Algorithm"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"wlpa"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Weighted Pagerank Algorithm"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"wpagerank"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Maximal independent set"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"mis"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Sybil Rank"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"sybilrank"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Subgraph Isomorphism"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"subgraph_isomorphism"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Motif"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"motif"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Kcliques"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"kcliques"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Ktruss"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"ktruss"})]}),(0,n.jsxs)(t.tr,{children:[(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"Leiden"}),(0,n.jsx)(t.td,{style:{textAlign:"center"},children:"leiden"})]})]})]}),"\n",(0,n.jsx)(t.h2,{id:"2basic-algorithms",children:"2.Basic algorithms"}),"\n",(0,n.jsx)(t.h3,{id:"21breadth-first-search",children:"2.1.Breadth-First Search"}),"\n",(0,n.jsxs)(t.p,{children:["Breadth first Search implements the Breadth-first Search algorithm, which starts from the root vertex and traverses all accessible vertices along the width of the graph. Returns the number of vertices traversed. Please refer to the algorithm content",(0,n.jsx)(t.a,{href:"https://en.wikipedia.org/wiki/Breadth-first_search",title:"bfs wiki",children:"https://en.wikipedia.org/wiki/Breadth-first_search"}),"\u3002"]}),"\n",(0,n.jsx)(t.h3,{id:"22pagerank",children:"2.2.Pagerank"}),"\n",(0,n.jsxs)(t.p,{children:["Web page sorting program to achieve the commonly used Pagerank algorithm. The algorithm calculates the importance ranking of all vertices according to the edge and edge weight in the graph. The higher the PageRank value, the higher the importance of the vertex in the graph. The reciprocal of the number of vertices is used as the initial Rank value of each vertex, and then the Rank value of the vertices is transferred to the adjacent vertices on average according to the outgoing edges, and the transfer process is repeated until the given convergence threshold is met or the given number of iterations is reached. At the end of each pass round, a certain proportion of the Rank values of all vertices will be randomly transferred to any vertex. Please refer to the algorithm content ",(0,n.jsx)(t.a,{href:"https://en.wikipedia.org/wiki/PageRank",title:"pagerank wiki",children:"https://en.wikipedia.org/wiki/PageRank"}),"\u3002"]}),"\n",(0,n.jsx)(t.h3,{id:"23single-source-shortest-path",children:"2.3.Single-Source Shortest Path"}),"\n",(0,n.jsxs)(t.p,{children:["The Single Source Shortest Path algorithm realizes the Single Source Shortest Path algorithm. It calculates the shortest path length from a given source vertex to any other vertex. Please refer to the algorithm content",(0,n.jsx)(t.a,{href:"https://en.wikipedia.org/wiki/Shortest_path_problem",title:"shortest path wiki",children:"https://en.wikipedia.org/wiki/Shortest_path_problem"}),"\u3002"]}),"\n",(0,n.jsx)(t.h3,{id:"24weakly-connected-components",children:"2.4.Weakly Connected Components"}),"\n",(0,n.jsxs)(t.p,{children:["The program of Weakly Connected Components has implemented the algorithm. It can calculate all the weakly connected components in the graph. A weakly connected component is a subgraph of a graph in which reachable paths exist between any two points. Please refer to the algorithm content",(0,n.jsxs)(t.a,{href:"https://en.wikipedia.org/wiki/Connected_component_(graph_theory)",title:"scc wiki",children:["https://en.wikipedia.org/wiki/Connected",(0,n.jsx)(t.em,{children:"component"}),"(graph_theory)"]}),"\u3002"]}),"\n",(0,n.jsx)(t.h3,{id:"25local-clustering-coefficient",children:"2.5.Local Clustering Coefficient"}),"\n",(0,n.jsxs)(t.p,{children:["The average Clustering Coefficient program implements the Local Clustering Coefficient algorithm to calculate the coefficient of the degree of clustering between vertices in the graph. The returned results include the overall clustering coefficient and the vertex clustering coefficient. The overall agglomeration coefficient reflects the evaluation of the overall agglomeration degree in the graph, and the vertex agglomeration coefficient includes the agglomeration coefficient of any vertex, which reflects the agglomeration degree near the vertex. The higher the agglomeration coefficient, the higher the agglomeration degree. Please refer to the algorithm content",(0,n.jsx)(t.a,{href:"https://en.wikipedia.org/wiki/Clustering_coefficient",title:"lcc wiki",children:"https://en.wikipedia.org/wiki/Clustering_coefficient"}),"\u3002"]}),"\n",(0,n.jsx)(t.h3,{id:"26label-propagation-algorithm",children:"2.6.Label Propagation Algorithm"}),"\n",(0,n.jsxs)(t.p,{children:["The program implements Label Propagation Algorithm. This algorithm is a community discovery algorithm based on tag propagation and computes unauthorised graphs. In label passing, each vertex adds up all the labels received, and randomly selects one of the labels with the highest sum. The iteration converges or the algorithm terminates after a given number of rounds. The final output is a label for each vertex, and vertices with the same label value are considered to be in the same community. Please refer to the algorithm content",(0,n.jsx)(t.a,{href:"https://en.wikipedia.org/wiki/Label_Propagation_Algorithm",title:"lpa wiki",children:"https://en.wikipedia.org/wiki/Label_Propagation_Algorithm"}),"\u3002"]}),"\n",(0,n.jsx)(t.h2,{id:"3extended-algorithms",children:"3.Extended algorithms"}),"\n",(0,n.jsx)(t.h3,{id:"31all-pair-shortest-path",children:"3.1.All-Pair Shortest Path"}),"\n",(0,n.jsxs)(t.p,{children:["The All-Pair Shortest Path program realizes the all-pair Shortest Path algorithm and calculates the shortest path between any two points in the graph. Returns the shortest path length between any pair of vertices where the path exists. Please refer to the algorithm content",(0,n.jsx)(t.a,{href:"https://en.wikipedia.org/wiki/Floyd-Warshall_algorithm",title:"Floyd-Warshall algorighm wiki",children:"https://en.wikipedia.org/wiki/Floyd-Warshall_algorithm"})]}),"\n",(0,n.jsx)(t.h3,{id:"32betweenness-centrality",children:"3.2.Betweenness Centrality"}),"\n",(0,n.jsxs)(t.p,{children:["Betweenness Centrality algorithm is implemented in Betweenness centrality program to estimate the betweenness centrality value of all vertices in a graph. The value of intermediate centrality reflects the possibility that any shortest path in the graph passes through the vertex, and the higher the value, the more shortest paths pass through the vertex. During calculation, the number of sampling points should be given, and the calculation should be carried out based on these sampling points. Please refer to the algorithm content",(0,n.jsx)(t.a,{href:"https://en.wikipedia.org/wiki/Betweenness_centrality",title:"bc wiki",children:"https://en.wikipedia.org/wiki/Betweenness_centrality"}),"\u3002"]}),"\n",(0,n.jsx)(t.h3,{id:"33belief-propagation",children:"3.3.Belief Propagation"}),"\n",(0,n.jsxs)(t.p,{children:["The Belief Propagation algorithm is implemented in the confidence propagation program. Given the edge distribution of the observed vertices, the algorithm estimates the edge distribution of the unobserved vertices by using the mechanism of passing messages among vertices. Please refer to the algorithm content",(0,n.jsx)(t.a,{href:"https://en.wikipedia.org/wiki/Belief_propagation",children:"https://en.wikipedia.org/wiki/Belief_propagation"}),"\u3002"]}),"\n",(0,n.jsx)(t.h3,{id:"34closeness-centrality",children:"3.4.Closeness Centrality"}),"\n",(0,n.jsxs)(t.p,{children:["The Distance Centrality program implements the Closeness Centrality algorithm, which estimates the average shortest path length from any node to other nodes in the graph. A smaller distance centrality value indicates that the node has a smaller average shortest distance to other nodes, meaning it is more centrally located in the graph from a geometric perspective. The calculation requires specifying the number of sampling points, and the algorithm computes the centrality values for each of these sampling points. For the algorithm content, please refer to ",(0,n.jsx)(t.a,{href:"https://en.wikipedia.org/wiki/Closeness_centrality",title:"clce wiki",children:"https://en.wikipedia.org/wiki/Closeness_centrality"}),"."]}),"\n",(0,n.jsx)(t.h3,{id:"35common-neighborhood",children:"3.5.Common Neighborhood"}),"\n",(0,n.jsx)(t.p,{children:"The Common Neighborhood program implements the common Neighborhood algorithm to count the number of common neighbors between any given pair of neighboring vertices. Given several pairs of vertices to be queried, the result is the number of common neighbors of any pair of vertices to be queried."}),"\n",(0,n.jsx)(t.h3,{id:"36degree-correlation",children:"3.6.Degree Correlation"}),"\n",(0,n.jsxs)(t.p,{children:["The Degree Correlation algorithm is implemented in the degree correlation program. It calculates the degree correlation degree of a graph by calculating the Pearson correlation coefficient between any adjacent vertex pairs, which can be used to characterize the correlation degree between high-degree vertices in the graph. A higher degree of correlation indicates a higher degree of correlation between vertices of higher degree in the graph. Please refer to the algorithm content",(0,n.jsx)(t.a,{href:"https://en.wikipedia.org/wiki/Pearson_correlation_coefficient",title:"dc wiki",children:"https://en.wikipedia.org/wiki/Pearson_correlation_coefficient"})]}),"\n",(0,n.jsx)(t.h3,{id:"37dimension-estimation",children:"3.7.Dimension Estimation"}),"\n",(0,n.jsxs)(t.p,{children:["The diameter Estimation program implements the Dimension Estimation algorithm. The algorithm calculates the length of the longest shortest path in the graph, which is used to characterize the diameter of the graph. Please refer to the algorithm content",(0,n.jsx)(t.a,{href:"http://mathworld.wolfram.com/GraphDiameter.html",title:"graph diameter",children:"http://mathworld.wolfram.com/GraphDiameter.html"}),"\u3002"]}),"\n",(0,n.jsx)(t.h3,{id:"38egonet",children:"3.8.EgoNet"}),"\n",(0,n.jsx)(t.p,{children:"The EgoNet algorithm requires a given root vertex and K value, and takes the root vertex as the source vertex to conduct a width-first search to find the subgraph composed of all neighbors within K degrees. The found subgraph is called the EgoNet of the root vertex."}),"\n",(0,n.jsx)(t.h3,{id:"39hyperlink-induced-topic-search",children:"3.9.Hyperlink-Induced Topic Search"}),"\n",(0,n.jsxs)(t.p,{children:["The Hyperlink Topic Search algorithm implements the Hyperlink-induced topic search algorithm, which assumes that each vertex has two attributes: Authority and Hub. A good hub vertex should point to many vertices with high authority. And a good authority vertex should be pointed to by many vertices of high pivot type. The algorithm returns the authority value and the hub value for each vertex. Please refer to the algorithm content",(0,n.jsx)(t.a,{href:"https://en.wikipedia.org/wiki/HITS_algorithm",title:"HITS algorithm",children:"https://en.wikipedia.org/wiki/HITS_algorithm"}),"\u3002"]}),"\n",(0,n.jsx)(t.h3,{id:"310jaccard-index",children:"3.10.Jaccard Index"}),"\n",(0,n.jsxs)(t.p,{children:["The Jaccard coefficient program implements the Jaccard Index algorithm. The algorithm calculates the Jaccard coefficient between a given pair of vertices, which can be used to represent the similarity of the two vertices. A higher Jaccard coefficient indicates a higher degree of similarity between pairs of vertices. Given several pairs of vertices with the query, the Jaccard coefficients of these pairs are returned. Please refer to the algorithm content",(0,n.jsx)(t.a,{href:"https://en.wikipedia.org/wiki/Jaccard_index",title:"ji wiki",children:"https://en.wikipedia.org/wiki/Jaccard_index"}),"\u3002"]}),"\n",(0,n.jsx)(t.h3,{id:"311k-core",children:"3.11.K-core"}),"\n",(0,n.jsxs)(t.p,{children:["k accounting method implements k-core algorithm. The algorithm computes the number of kernels of all vertices, or finds all K-nucleon graphs in the graph. K-nucleon graph is a special subgraph in which the degree of any vertex is not less than a given K value. Please refer to the algorithm content",(0,n.jsx)(t.a,{href:"https://en.wikipedia.org/wiki/Degeneracy_(graph_theory)",title:"kcore wiki",children:"https://en.wikipedia.org/wiki/Degeneracy_(graph_theory)"}),"\u3002"]}),"\n",(0,n.jsx)(t.h3,{id:"312louvain",children:"3.12.Louvain"}),"\n",(0,n.jsxs)(t.p,{children:["The Louvain community discovery program implements the Fast-unfolding algorithm. The algorithm is a community discovery algorithm based on modularity. It maximizes the modularity of the graph by constantly merging vertex communities, and can discover the hierarchical community structure. Please refer to the algorithm content ",(0,n.jsx)(t.a,{href:"https://en.wikipedia.org/wiki/Louvain_Modularity",title:"louvain modularity wiki",children:"https://en.wikipedia.org/wiki/Louvain_Modularity"}),"\u3002"]}),"\n",(0,n.jsx)(t.h3,{id:"313multiple-source-shortest-paths",children:"3.13.Multiple-source Shortest Paths"}),"\n",(0,n.jsxs)(t.p,{children:["The multisource Shortest Paths program implements the multiple-source Shortest Paths algorithm to calculate the shortest path value to any vertex from the given Multiple source vertices. Where, the shortest path value of multiple source vertices to a vertex is the minimum value of the shortest path from each source vertex to the vertex. Please refer to the algorithm content",(0,n.jsx)(t.a,{href:"https://en.wikipedia.org/wiki/Shortest_path_problem",title:"shortest path wiki",children:"https://en.wikipedia.org/wiki/Shortest_path_problem"}),"\u3002"]}),"\n",(0,n.jsx)(t.h3,{id:"314personalized-pagerank",children:"3.14.Personalized PageRank"}),"\n",(0,n.jsxs)(t.p,{children:["Personalized web ranking program has Personalized PageRank algorithm. According to the given source vertex, the algorithm calculates the importance ranking of all vertices to the source vertex based on the source vertex. The higher the Rank value, the more important the vertex is to the source vertex. Unlike PageRank, the source vertex Rank value is 1, the rest of the vertex Rank value is 0; And a certain proportion of Rank value will be immediately transferred back to the source vertex after each round of transmission. Please refer to the algorithm content ",(0,n.jsx)(t.a,{href:"https://cs.stanford.edu/people/plofgren/Fast-PPR_KDD_Talk.pdf",children:"https://cs.stanford.edu/people/plofgren/Fast-PPR_KDD_Talk.pdf"}),"\u3002"]}),"\n",(0,n.jsx)(t.h3,{id:"315strongly-connected-components",children:"3.15.Strongly Connected Components"}),"\n",(0,n.jsxs)(t.p,{children:["Strongly Connected component program realizes the Strongly Connected Components algorithm. The algorithm computes all strongly connected components of the graph, which is a subgraph of the graph that can start from any vertex to any other vertex. Please refer to the algorithm content",(0,n.jsx)(t.a,{href:"https://en.wikipedia.org/wiki/Strongly_connected_component",title:"scc wiki",children:"https://en.wikipedia.org/wiki/Strongly_connected_component"}),"\u3002"]}),"\n",(0,n.jsx)(t.h3,{id:"316speaker-listener-label-propagation-algorithm",children:"3.16.Speaker-listener Label Propagation Algorithm"}),"\n",(0,n.jsx)(t.p,{children:"The program realizes the Speaker-listener Label Propagation Algorithm. This algorithm is a community discovery algorithm based on tag propagation and historical tag record, which is an extension of tag propagation algorithm. Different from the label propagation algorithm, this algorithm records the historical labels of all vertices. When the labels are accumulated in the iteration, the historical labels are also counted. The final output is all the historical label records for each vertex. Please refer to the paper for the algorithm content:\u201cSLPA: Uncovering Overlapping Communities in Social Networks via a Speaker-Listener Interaction Dynamic Process\u201d\u3002"}),"\n",(0,n.jsx)(t.h3,{id:"317single-pair-shortest-path",children:"3.17.Single-Pair Shortest Path"}),"\n",(0,n.jsxs)(t.p,{children:["The program of the shortest path between two points implements the Bidirectional point-first Search algorithm. It searches forward width First along the outgoing edge from the starting point and reverse width first along the incoming edge from the end point on the directed undirected graph. The shortest path length from the starting point to the ending point is determined by traversing the vertices of the starting point and the ending point together. Please refer to the algorithm content",(0,n.jsx)(t.a,{href:"https://en.wikipedia.org/wiki/Bidirectional_search",title:"Bidirectional search",children:"https://en.wikipedia.org/wiki/Bidirectional_search"}),"\u3002"]}),"\n",(0,n.jsx)(t.h3,{id:"318triangle-counting",children:"3.18.Triangle Counting"}),"\n",(0,n.jsx)(t.p,{children:'The Triangle-counting algorithm is implemented to calculate the number of triangles in an undirected graph, which can be used to characterize the correlation degree of the vertices in the graph. The higher the number of triangles, the higher the degree of correlation of the vertices in the graph. For the algorithm content, please refer to the paper, "Finding, Counting and Listing All Triangles in Large Graphs, an Experimental Study".'}),"\n",(0,n.jsx)(t.h3,{id:"319trustrank",children:"3.19.Trustrank"}),"\n",(0,n.jsxs)(t.p,{children:["The Trust Index Sorting Algorithm implements the Trustrank algorithm, which can calculate the Trust Index of any vertex based on a given whitelist. The higher the trust score, the less likely the vertex is to be illegal. Please refer to the algorithm content for details. ",(0,n.jsx)(t.a,{href:"https://en.wikipedia.org/wiki/TrustRank",title:"trustrank wiki",children:"https://en.wikipedia.org/wiki/TrustRank"}),"\u3002"]}),"\n",(0,n.jsx)(t.h3,{id:"320weighted-label-propagation-algorithm",children:"3.20.Weighted Label Propagation Algorithm"}),"\n",(0,n.jsxs)(t.p,{children:["Weighted Label Propagation Algorithm is implemented in the program. = Different from the label propagation algorithm, the label transmission is related to the weight of the edge. During label transmission, the weight of each vertex will be accumulated according to the incoming edge of the label, and the label with the highest sum will be randomly selected. Please refer to the algorithm content ",(0,n.jsx)(t.a,{href:"https://en.wikipedia.org/wiki/Label_Propagation_Algorithm",title:"lpa wiki",children:"https://en.wikipedia.org/wiki/Label_Propagation_Algorithm"}),"\u3002"]}),"\n",(0,n.jsx)(t.h3,{id:"321weighted-pagerank-algorithm",children:"3.21.Weighted Pagerank Algorithm"}),"\n",(0,n.jsxs)(t.p,{children:["Weighted Pagerank is implemented in the weighted Pagerank algorithm. Different from PageRank algorithm, the transfer of Rank value is related to the weight of the edge. The Rank value of the vertex will be transferred to the adjacent vertices according to the weight of the edge. Please refer to the algorithm content",(0,n.jsx)(t.a,{href:"https://en.wikipedia.org/wiki/PageRank",children:"https://en.wikipedia.org/wiki/PageRank"}),"\u3002"]}),"\n",(0,n.jsx)(t.h3,{id:"322maximal-independent-set",children:"3.22.Maximal independent set"}),"\n",(0,n.jsxs)(t.p,{children:["Maximal independent set algorithm implements Maximal Independent Set algorithm. A maximum independent set means that there are no vertices outside the independent set that can join it. A graph may have many MIS that vary greatly in size, and the algorithm finds one of them. Please refer to the algorithm content ",(0,n.jsx)(t.a,{href:"https://en.wikipedia.org/wiki/Maximal_independent_set#Sequential_algorithm",title:"Maximal independent set wiki",children:"https://en.wikipedia.org/wiki/Maximal_independent_set#Sequential_algorithm"}),"\u3002"]}),"\n",(0,n.jsx)(t.h3,{id:"323sybil-rank",children:"3.23.Sybil Rank"}),"\n",(0,n.jsx)(t.p,{children:"Sybil detection algorithm implements Sybil Rank algorithm. The SybilRank algorithm starts from non-Sybil nodes and performs a random walk with premature termination. Please refer to the paper for the algorithm content:\u201cAiding the Detection of Fake Accounts in Large Scale Social Online Services\u201d\u3002"}),"\n",(0,n.jsx)(t.h3,{id:"324subgraph-isomorphism",children:"3.24.Subgraph Isomorphism"}),"\n",(0,n.jsxs)(t.p,{children:["The subgraph matching algorithm implements the subgraph_isomorphism algorithm. The subgraph_isomorphism algorithm matches all nodes in the whole graph and outputs the number of times each node is matched. Algorithm Content Reference ",(0,n.jsx)(t.a,{href:"https://www.jsjkx.com/CN/article/openArticlePDF.jsp?id=18105",children:"https://www.jsjkx.com/CN/article/openArticlePDF.jsp?id=18105"})]}),"\n",(0,n.jsx)(t.h3,{id:"325motif",children:"3.25.Motif"}),"\n",(0,n.jsxs)(t.p,{children:["Pattern matching algorithm implements motif algorithm. motif algorithm matches k-order subgraphs for the specified nodes, and finally outputs the number of each kind of K-order subgraphs for each specified node. Each K-order subgraph is represented by a 64-bit integer, and the $i \\times k + j$bit is 1, indicating that there is an edge i->j in the subgraph. Algorithm Content Reference ",(0,n.jsx)(t.a,{href:"https://en.wikipedia.org/wiki/Network_motif#mfinder",children:"https://en.wikipedia.org/wiki/Network_motif#mfinder"})]}),"\n",(0,n.jsx)(t.h3,{id:"326kcliques",children:"3.26.Kcliques"}),"\n",(0,n.jsxs)(t.p,{children:["The K-order clique counting algorithm implements the k-cliques algorithm. The k-cliques algorithm calculates the number of all K-order complete subgraphs in the graph, and finally outputs the number of k-order complete subgraphs of each node. Algorithm Content Reference ",(0,n.jsx)(t.a,{href:"https://en.wikipedia.org/wiki/Clique_problem#Cliques_of_fixed_size",children:"https://en.wikipedia.org/wiki/Clique_problem#Cliques_of_fixed_size"})]}),"\n",(0,n.jsx)(t.h3,{id:"327ktruss",children:"3.27.Ktruss"}),"\n",(0,n.jsxs)(t.p,{children:["The k-order truss counting algorithm implements the k-truss algorithm. A k-truss is a subgraph in which each edge is the edge of at least k-2 triangles. The k-truss algorithm finds out the k-truss subgraph of the graph, and finally outputs the neighbor node list of each node in the subgraph. Algorithm Content Reference",(0,n.jsx)(t.a,{href:"https://louridas.github.io/rwa/assignments/finding-trusses/",children:"https://louridas.github.io/rwa/assignments/finding-trusses/"})]}),"\n",(0,n.jsx)(t.h3,{id:"328leiden",children:"3.28.Leiden"}),"\n",(0,n.jsxs)(t.p,{children:["Leiden algorithm implements Leiden's algorithm. leiden algorithm is a community discovery algorithm based on modularity. Compared with louvain algorithm, leiden algorithm has the advantage that Leiden algorithm detects the broken links in the community to ensure that each community has good connectivity. Algorithm Content Reference ",(0,n.jsx)(t.a,{href:"https://www.nature.com/articles/s41598-019-41695-z#Sec4",children:"https://www.nature.com/articles/s41598-019-41695-z#Sec4"})]})]})}function d(e={}){const{wrapper:t}={...(0,r.R)(),...e.components};return t?(0,n.jsx)(t,{...e,children:(0,n.jsx)(c,{...e})}):c(e)}}}]);
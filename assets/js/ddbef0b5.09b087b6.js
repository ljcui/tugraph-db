"use strict";(self.webpackChunkdocusaurus=self.webpackChunkdocusaurus||[]).push([[44033],{28453:(e,n,t)=>{t.d(n,{R:()=>o,x:()=>s});var a=t(96540);const i={},r=a.createContext(i);function o(e){const n=a.useContext(r);return a.useMemo((function(){return"function"==typeof e?e(n):{...n,...e}}),[n,e])}function s(e){let n;return n=e.disableParentContext?"function"==typeof e.components?e.components(i):e.components||i:o(e.components),a.createElement(r.Provider,{value:n},e.children)}},73130:(e,n,t)=>{t.r(n),t.d(n,{assets:()=>l,contentTitle:()=>o,default:()=>u,frontMatter:()=>r,metadata:()=>s,toc:()=>d});var a=t(74848),i=t(28453);const r={},o="OlapOnDB API",s={id:"developer-manual/interface/olap/olap-on-db-api",title:"OlapOnDB API",description:"This document provides detailed instructions for using the OlapOnDB API",source:"@site/versions/version-4.1.0/en-US/source/5.developer-manual/6.interface/2.olap/3.olap-on-db-api.md",sourceDirName:"5.developer-manual/6.interface/2.olap",slug:"/developer-manual/interface/olap/olap-on-db-api",permalink:"/tugraph-db/en/4.1.0/developer-manual/interface/olap/olap-on-db-api",draft:!1,unlisted:!1,tags:[],version:"current",sidebarPosition:3,frontMatter:{},sidebar:"tutorialSidebar",previous:{title:"OlapBase API",permalink:"/tugraph-db/en/4.1.0/developer-manual/interface/olap/olap-base-api"},next:{title:"OlapOnDisk API",permalink:"/tugraph-db/en/4.1.0/developer-manual/interface/olap/olap-on-disk-api"}},l={},d=[{value:"1.Introduction",id:"1introduction",level:2},{value:"2.Schema",id:"2schema",level:2},{value:"2.1.Snapshot Based Storage Structure",id:"21snapshot-based-storage-structure",level:3},{value:"2.2.BSP Calculation Model",id:"22bsp-calculation-model",level:3},{value:"3.Algorithm example",id:"3algorithm-example",level:2},{value:"3.1.Main function",id:"31main-function",level:3},{value:"3.2.PageRank Algorithm process",id:"32pagerank-algorithm-process",level:3},{value:"4.1.Transaction creation",id:"41transaction-creation",level:3},{value:"4.2.Parallelization to create a directed graph",id:"42parallelization-to-create-a-directed-graph",level:3},{value:"4.3.Create undirected graphs in parallel",id:"43create-undirected-graphs-in-parallel",level:3},{value:"4.4.Obtain the output degree",id:"44obtain-the-output-degree",level:3},{value:"4.5.Obtain the input degree",id:"45obtain-the-input-degree",level:3},{value:"4.6.Gets the outgoing edge set",id:"46gets-the-outgoing-edge-set",level:3},{value:"4.7.Gets the incoming edge set",id:"47gets-the-incoming-edge-set",level:3},{value:"4.8.Get the node ID of OlapOnDB corresponding to the node in TuGraph",id:"48get-the-node-id-of-olapondb-corresponding-to-the-node-in-tugraph",level:3},{value:"4.9.Gets the node number of TuGraph corresponding to a node in OlapOnDB",id:"49gets-the-node-number-of-tugraph-corresponding-to-a-node-in-olapondb",level:3},{value:"4.10.Description of active vertices",id:"410description-of-active-vertices",level:3}];function c(e){const n={blockquote:"blockquote",code:"code",h1:"h1",h2:"h2",h3:"h3",header:"header",li:"li",ol:"ol",p:"p",pre:"pre",...(0,i.R)(),...e.components};return(0,a.jsxs)(a.Fragment,{children:[(0,a.jsx)(n.header,{children:(0,a.jsx)(n.h1,{id:"olapondb-api",children:"OlapOnDB API"})}),"\n",(0,a.jsxs)(n.blockquote,{children:["\n",(0,a.jsx)(n.p,{children:"This document provides detailed instructions for using the OlapOnDB API"}),"\n"]}),"\n",(0,a.jsx)(n.h2,{id:"1introduction",children:"1.Introduction"}),"\n",(0,a.jsx)(n.p,{children:"Generally speaking, users need to implement the process of subgraph extraction by themselves. Then use the rich interface in TuGraph to implement your own graph analysis algorithm."}),"\n",(0,a.jsx)(n.p,{children:"This document mainly introduces the Procedure and Embed interface design, and introduces some common interface, specific interface information see include/lgraph/olap_on_db.h file."}),"\n",(0,a.jsx)(n.h2,{id:"2schema",children:"2.Schema"}),"\n",(0,a.jsx)(n.p,{children:"Procedure and Embed auxiliary functions are mainly included in the OlapOnDB class, and some more frequently used functions will be introduced one by one"}),"\n",(0,a.jsx)(n.p,{children:"OLAP is associated with the following common data structures in TuGraph:"}),"\n",(0,a.jsxs)(n.ol,{children:["\n",(0,a.jsxs)(n.li,{children:["\n",(0,a.jsxs)(n.p,{children:["DB graph analysis class ",(0,a.jsx)(n.code,{children:"OlapOnDB"})]}),"\n"]}),"\n",(0,a.jsxs)(n.li,{children:["\n",(0,a.jsxs)(n.p,{children:["Vertex array ",(0,a.jsx)(n.code,{children:"ParallelVector"})]}),"\n"]}),"\n",(0,a.jsxs)(n.li,{children:["\n",(0,a.jsxs)(n.p,{children:["Vertex set ",(0,a.jsx)(n.code,{children:"ParallelBitset"})]}),"\n"]}),"\n",(0,a.jsxs)(n.li,{children:["\n",(0,a.jsxs)(n.p,{children:["Side data structure ",(0,a.jsx)(n.code,{children:"AdjUnit/AdjUnit"})]}),"\n"]}),"\n",(0,a.jsxs)(n.li,{children:["\n",(0,a.jsxs)(n.p,{children:["Edge collection data structure ",(0,a.jsx)(n.code,{children:"AdjList"})]}),"\n"]}),"\n"]}),"\n",(0,a.jsx)(n.h3,{id:"21snapshot-based-storage-structure",children:"2.1.Snapshot Based Storage Structure"}),"\n",(0,a.jsx)(n.p,{children:'The OlapOnDB class in TuGraph provides a data "snapshot," that is, a fully usable copy of a given data set that includes a mirror image of the corresponding data at a certain point in time (the point at which the copy was started). Because OLAP operations involve only reads and not writes, OlapOnDB arranges data in a more compact manner, saving space while improving data access locality.'}),"\n",(0,a.jsx)(n.h3,{id:"22bsp-calculation-model",children:"2.2.BSP Calculation Model"}),"\n",(0,a.jsx)(n.p,{children:"TuGraph uses the BSP (Bulk Synchronous Parallel) model in the process of calculation, which makes the process can be executed in parallel, and greatly improves the efficiency of the program."}),"\n",(0,a.jsx)(n.p,{children:"The core idea of BSP calculation model is to propose and use Super Step. After OlapOnDB is created, the computation on this data is divided into multiple supersteps, such as PageRank, which is divided into multiple iterations, and each iteration is a Super Step. There is explicit synchronization between different Super Steps to ensure that all threads proceed to the next Super Step at the same time after completing the same superstep. Within a Super Step, all threads execute asynchronously, using parallelism to improve computational efficiency."}),"\n",(0,a.jsx)(n.p,{children:"BSP calculation model can effectively avoid deadlock, and achieve coarse-grained global synchronization in hardware mode by means of obstacle synchronization, so that graph computation can be executed in parallel, and programmers do not need to spend much time on synchronization mutual exclusion."}),"\n",(0,a.jsx)(n.h2,{id:"3algorithm-example",children:"3.Algorithm example"}),"\n",(0,a.jsx)(n.p,{children:"Here is an explanation of the PageRank algorithm, which is mainly divided into the main function Process and the PageRank algorithm process function."}),"\n",(0,a.jsx)(n.h3,{id:"31main-function",children:"3.1.Main function"}),"\n",(0,a.jsx)(n.p,{children:"The main function has three input parameters, 'TuGraph' database parameter 'db', the request 'request' obtained from the web side, and the return value 'response' given to the web side. The overall process can be divided into the following steps:"}),"\n",(0,a.jsxs)(n.ol,{children:["\n",(0,a.jsxs)(n.li,{children:["\n",(0,a.jsx)(n.p,{children:"Obtain related parameters"}),"\n"]}),"\n",(0,a.jsxs)(n.li,{children:["\n",(0,a.jsx)(n.p,{children:"Create a snapshot class"}),"\n"]}),"\n",(0,a.jsxs)(n.li,{children:["\n",(0,a.jsx)(n.p,{children:"Main process of PageRank algorithm"}),"\n"]}),"\n",(0,a.jsxs)(n.li,{children:["\n",(0,a.jsx)(n.p,{children:"Obtain and send the return value of the web page"}),"\n"]}),"\n"]}),"\n",(0,a.jsx)(n.pre,{children:(0,a.jsx)(n.code,{className:"language-c",children:'extern "C" bool Process(GraphDB & db, const std::string & request, std::string & response) {\n\n    // Obtain the number of iterations from web side requests (num_iterations)\uff0c\n    int num_iterations = 20;\n    try {\n        json input = json::parse(request);\n        num_iterations = input["num_iterations"].get<int>();\n    } catch (std::exception & e) {\n        throw std::runtime_error("json parse error");\n        return false;\n    }\n\n    // Read transaction creation and snapshot class creation\n    auto txn = db.CreateReadTxn();\n    OlapOnDB<Empty> olapondb(\n        db,\n        txn,\n        SNAPSHOT_PARALLEL\n    );\n\n    // Create a pr array to store the pr value for each node\n    ParallelVector<double> pr = olapondb.AllocVertexArray<double>();\n    // pagerank algorithm main process, obtain the pagerank value of each node\n    PageRankCore(olapondb, num_iterations, pr);\n\n    auto all_vertices = olapondb.AllocVertexSubset();\n    all_vertices.Fill();\n    /*\n        Function Purpose: Gets the number of the node with the largest pagerank among all nodes\n\n        Function flow description: This function executes Func A for node vi (also known as the active vertices) corresponding to all bits of 1 in the vertex set all_vertices. The return value of Func A is then used as the second input parameter of Func B to obtain the local maximum value (because the first input parameter is 0. So the return value is actually the pagerank value of each node). Finally, the return value of all threads is summarized, and Func B is executed again to get the global return value, and stored in the max_pr_vi variable\n    */\n    size_t max_pr_vi = olapondb.ProcessVertexActive<size_t>(\n\n        //Func A\n        [&](size_t vi) {\n            return vi;\n        },\n        all_vertices,\n        0,\n\n        //Func B\n        [&](size_t a, size_t b) {\n            return pr[a] > pr[b] ? a : b;\n        }\n    );\n\n    // Retrieve and send the return value from the web page\n    json output;\n    output["max_pr_vid"] = olapondb.OriginalVid(max_pr_vi);\n    output["max_pr_val"] = pr[max_pr_vi];\n    response = output.dump();\n    return true;\n}\n'})}),"\n",(0,a.jsx)(n.h3,{id:"32pagerank-algorithm-process",children:"3.2.PageRank Algorithm process"}),"\n",(0,a.jsxs)(n.p,{children:[(0,a.jsx)(n.code,{children:"pagerank"})," main process has two input parameters, snapshot class (subgraph) and iteration times, the overall process can be divided into the following steps:"]}),"\n",(0,a.jsxs)(n.ol,{children:["\n",(0,a.jsx)(n.li,{children:"Initialization of related data structures"}),"\n",(0,a.jsx)(n.li,{children:"Initialize the pagerank value of each node"}),"\n",(0,a.jsx)(n.li,{children:"Calculation of pagerank value of each node, active vertices for all vertices (means that all vertices need to calculate pagerank value)"}),"\n",(0,a.jsx)(n.li,{children:"Obtain the pagerank value after 'num_iterations' of each node"}),"\n"]}),"\n",(0,a.jsx)(n.pre,{children:(0,a.jsx)(n.code,{className:"language-c",children:'void PageRankCore(OlapBase<Empty>& graph, int num_iterations, ParallelVector<double>& curr) {\n\n    // Initialization of related data structures\n    auto all_vertices = olapondb.AllocVertexSubset();\n    all_vertices.Fill();\n    auto curr = olapondb.AllocVertexArray<double>();\n    auto next = olapondb.AllocVertexArray<double>();\n    size_t num_vertices = olapondb.NumVertices();\n    double one_over_n = (double)1 / num_vertices;\n\n    // The initialization of the pagerank value of each node is inversely proportional to the degree of the node\n    double delta = graph.ProcessVertexActive<double>(\n        [&](size_t vi) {\n            curr[vi] = one_over_n;\n            if (olapondb.OutDegree(vi) > 0) {\n                curr[vi] /= olapondb.OutDegree(vi);\n            }\n            return one_over_n;\n        },\n        all_vertices);\n\n    // Total iteration process\n    double d = (double)0.85;\n        for (int ii = 0;ii < num_iterations;ii ++) {\n        printf("delta(%d)=%lf\\n", ii, delta);\n        next.Fill((double)0);\n\n        /*\n            Function Purpose: Calculates the pagerank of all nodes\n\n            Function flow description: This function is used to calculate the pagerank value of all nodes. Execute Func C on node vi corresponding to all the bits of 1 in all_vertices to obtain the pagerank value of vi in the current iteration and return the pagerank change value of vi node. The total change value of all active nodes is finally summarized and returned through the internal processing of the function, which is stored in the delta variable\n        */\n        delta = graph.ProcessVertexActive<double>(\n            // Func C\n            [&](size_t vi) {\n                double sum = 0;\n\n                // Gets the pagerank value of the current node from its neighbor\n                for (auto & edge : olapondb.InEdges(vi)) {\n                    size_t src = edge.neighbour;\n                    sum += curr[src];\n                }\n                next[vi] = sum;\n\n                // pagerank value calculation core formula\n                next[vi] = (1 - d) * one_over_n + d * next[vi];\n                if (ii == num_iterations - 1) {\n                    return (double)0;\n                } else {\n\n                    // Statistics of relevant intermediate variables\n                    if (olapondb.OutDegree(vi) > 0) {\n                        next[vi] /= olapondb.OutDegree(vi);\n                        return fabs(next[vi] - curr[vi]) * olapondb.OutDegree(vi);\n                    } else {\n                        return fabs(next[vi] - curr[vi]);\n                    }\n                }\n            },\n            all_vertices\n        );\n\n        // The pagerank value obtained in this iteration is output as the input of the next iteration\n        curr.Swap(next);\n    }\n}\n'})}),"\n",(0,a.jsx)(n.h3,{id:"41transaction-creation",children:"4.1.Transaction creation"}),"\n",(0,a.jsx)(n.pre,{children:(0,a.jsx)(n.code,{className:"language-c",children:"// Create a read transaction\nauto txn = db.CreateReadTxn();\n\n// Write transaction creation\nauto txn = db.CreateWriteTxn();\n"})}),"\n",(0,a.jsx)(n.h3,{id:"42parallelization-to-create-a-directed-graph",children:"4.2.Parallelization to create a directed graph"}),"\n",(0,a.jsx)(n.pre,{children:(0,a.jsx)(n.code,{className:"language-c",children:"OlapOnDB<empty> olapondb(</empty>\ndb,\ntxn,\nSNAPSHOT_PARALLEL\n)\n"})}),"\n",(0,a.jsx)(n.h3,{id:"43create-undirected-graphs-in-parallel",children:"4.3.Create undirected graphs in parallel"}),"\n",(0,a.jsx)(n.pre,{children:(0,a.jsx)(n.code,{className:"language-c",children:"OlapOnDB<empty> olapondb(</empty>\ndb,\ntxn,\nSNAPSHOT_PARALLEL | SNAPSHOT_UNDIRECTED;\n)\n"})}),"\n",(0,a.jsx)(n.h3,{id:"44obtain-the-output-degree",children:"4.4.Obtain the output degree"}),"\n",(0,a.jsx)(n.pre,{children:(0,a.jsx)(n.code,{className:"language-c",children:"size_t OutDegree(size_t vid)\n"})}),"\n",(0,a.jsx)(n.h3,{id:"45obtain-the-input-degree",children:"4.5.Obtain the input degree"}),"\n",(0,a.jsx)(n.pre,{children:(0,a.jsx)(n.code,{className:"language-c",children:"size_t InDegree(size_t vid)\n"})}),"\n",(0,a.jsx)(n.h3,{id:"46gets-the-outgoing-edge-set",children:"4.6.Gets the outgoing edge set"}),"\n",(0,a.jsx)(n.pre,{children:(0,a.jsx)(n.code,{className:"language-c",children:'/*\n    Function name: AdjList OutEdges(size_t vid)\n\n    Data structure:\n\n    An AdjList can be understood as an array of structures of type AdjUnit\n\n    AdjUnit has two member variables: 1. size_t neighbour 2. edge_data: neighbour indicates the number of the target node pointed by the outgoing edge. If the value is a licensed graph, the data type of edge_data is the same as the weight value of the edge in the input file\n\n    Example Output all outgoing neighbors of node vid\n*/\nfor (auto & edge : olapondb.OutEdges(vid)) {\n    size_t dst = edge.neighbour;\n    printf("src = %lu,dst = %lu\\n",vid,dst);\n}\n'})}),"\n",(0,a.jsx)(n.h3,{id:"47gets-the-incoming-edge-set",children:"4.7.Gets the incoming edge set"}),"\n",(0,a.jsx)(n.pre,{children:(0,a.jsx)(n.code,{className:"language-c",children:'AdjList<edgedata> InEdges(size_t vid)</edgedata>\n\n// Example: Outputs all inbound neighbors of node vid\nfor (auto & edge : olapondb.InEdges(vid)) {\nsize_t dst = edge.neighbour;\nprintf("src = %lu,dst = %lu\\n",vid,dst);\n}\n'})}),"\n",(0,a.jsx)(n.h3,{id:"48get-the-node-id-of-olapondb-corresponding-to-the-node-in-tugraph",children:"4.8.Get the node ID of OlapOnDB corresponding to the node in TuGraph"}),"\n",(0,a.jsx)(n.pre,{children:(0,a.jsx)(n.code,{className:"language-c",children:"size_t OriginalVid(size_t vid)\n\n// Note: The node numbers entered in TuGraph can be non-numbers, such as names. When OlapOnDB subgraphs are generated, names and other names will be converted to numbers for subsequent processing. Therefore, this method may not be applicable to some specific scenarios\n"})}),"\n",(0,a.jsx)(n.h3,{id:"49gets-the-node-number-of-tugraph-corresponding-to-a-node-in-olapondb",children:"4.9.Gets the node number of TuGraph corresponding to a node in OlapOnDB"}),"\n",(0,a.jsx)(n.pre,{children:(0,a.jsx)(n.code,{className:"language-c",children:"size_t MappedVid(size_t original_vid)\n"})}),"\n",(0,a.jsx)(n.h3,{id:"410description-of-active-vertices",children:"4.10.Description of active vertices"}),"\n",(0,a.jsx)(n.p,{children:"Active vertices refer to the vertices that need to be processed in the batch function. In this example, we simply output the number of active vertices and summarize the number of active vertices:"}),"\n",(0,a.jsx)(n.pre,{children:(0,a.jsx)(n.code,{className:"language-c",children:'ParallelBitset temp = 000111; // The current active vertices are vertices 3, 4 and 5\n\nsize_t delta = ForEachActiveVertex<double>(</double>\n//void c\n[&](size_t vi) {\nprintf("active_vertexId = %lu\\n",vi);\nreturn 1;\n},\nall_vertices\n);\n'})}),"\n",(0,a.jsx)(n.p,{children:"The result of this function is obvious. Because of multiple threads, the output order may vary:"}),"\n",(0,a.jsx)(n.pre,{children:(0,a.jsx)(n.code,{children:"active_vertexId = 3\nactive_vertexId = 4\nactive_vertexId = 5\n"})}),"\n",(0,a.jsx)(n.p,{children:"The local return value is 1. This function will add all the local return values in a thread-safe manner to the final return value, which is stored in the delta variable, and the final value is 3"})]})}function u(e={}){const{wrapper:n}={...(0,i.R)(),...e.components};return n?(0,a.jsx)(n,{...e,children:(0,a.jsx)(c,{...e})}):c(e)}}}]);
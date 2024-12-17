"use strict";(self.webpackChunkdocusaurus=self.webpackChunkdocusaurus||[]).push([[68522],{98540:(e,t,n)=>{n.r(t),n.d(t,{assets:()=>c,contentTitle:()=>o,default:()=>h,frontMatter:()=>i,metadata:()=>s,toc:()=>l});var a=n(74848),r=n(28453);const i={},o="Multi Level Interfaces",s={id:"introduction/characteristics/multi-level-Interfaces",title:"Multi Level Interfaces",description:"This document mainly introduces the design concept of TuGraph's multi-level interfaces.",source:"@site/versions/version-4.3.0/en-US/source/2.introduction/5.characteristics/2.multi-level-Interfaces.md",sourceDirName:"2.introduction/5.characteristics",slug:"/introduction/characteristics/multi-level-Interfaces",permalink:"/tugraph-db/en/4.3.0/introduction/characteristics/multi-level-Interfaces",draft:!1,unlisted:!1,tags:[],version:"current",sidebarPosition:2,frontMatter:{},sidebar:"tutorialSidebar",previous:{title:"Performance Oriented",permalink:"/tugraph-db/en/4.3.0/introduction/characteristics/performance-oriented"},next:{title:"HTAP",permalink:"/tugraph-db/en/4.3.0/introduction/characteristics/htap"}},c={},l=[{value:"1.Introduction",id:"1introduction",level:2},{value:"2.Client Interface",id:"2client-interface",level:3},{value:"3.Server Interface",id:"3server-interface",level:3}];function d(e){const t={blockquote:"blockquote",h1:"h1",h2:"h2",h3:"h3",header:"header",img:"img",p:"p",strong:"strong",...(0,r.R)(),...e.components};return(0,a.jsxs)(a.Fragment,{children:[(0,a.jsx)(t.header,{children:(0,a.jsx)(t.h1,{id:"multi-level-interfaces",children:"Multi Level Interfaces"})}),"\n",(0,a.jsxs)(t.blockquote,{children:["\n",(0,a.jsx)(t.p,{children:"This document mainly introduces the design concept of TuGraph's multi-level interfaces."}),"\n"]}),"\n",(0,a.jsx)(t.h2,{id:"1introduction",children:"1.Introduction"}),"\n",(0,a.jsx)(t.p,{children:"The multi-level interface is a balance between usability and high performance that TuGraph has designed to meet a variety of use cases. For example, Cypher, a declarative graph query language, can abstract away the implementation details of a graph database and express queries based on the graph model. However, Cypher's high-level descriptions cannot be efficiently translated into low-level execution, so TuGraph provides a procedural language, Procedure API, to achieve optimal database performance."}),"\n",(0,a.jsx)(t.p,{children:"Interfaces can be roughly divided into client interfaces and server interfaces. Most operations are performed on the server, and the client only does data encapsulation and parsing. The client and server are connected through a network, and TuGraph supports both flexible short-connection REST protocol and more efficient long-connection RPC protocol, which can be selected according to different business scenarios."}),"\n",(0,a.jsx)(t.p,{children:"The server interfaces are all at the calculation layer and are logically separated from the graph data storage by a Core API layer."}),"\n",(0,a.jsx)(t.p,{children:(0,a.jsx)(t.img,{alt:"Multi Level Interfaces",src:n(85780).A+"",width:"742",height:"476"})}),"\n",(0,a.jsx)(t.h3,{id:"2client-interface",children:"2.Client Interface"}),"\n",(0,a.jsx)(t.p,{children:"The client interface refers to the interface executed on the client and is typically used for integration into software applications. TuGraph's client interface is relatively simple, including login/logout, data import/export, stored procedure loading and calling, and Cypher operations. Cypher integrates most of the functions, including data operations, graph model operations, operations and account management."}),"\n",(0,a.jsx)(t.p,{children:"Since the parameters and return values of Cypher are strings, JAVA OGM is a structured wrapper for Cypher, meaning that query results can be encapsulated into a typed object for ease of use."}),"\n",(0,a.jsx)(t.h3,{id:"3server-interface",children:"3.Server Interface"}),"\n",(0,a.jsx)(t.p,{children:"The server interface includes Cypher, Procedure API, OLAP API, and GNN PI, which provide services for graph transaction engines, graph analysis engines, and graph neural network engines. The characteristics of each interface are explained in detail below."}),"\n",(0,a.jsxs)(t.blockquote,{children:["\n",(0,a.jsxs)(t.p,{children:[(0,a.jsx)(t.strong,{children:"Cypher"})," is an abstract description of query logic that is independent of execution logic and is more user-friendly for graph database applications, similar to the SQL language of relational databases. TuGraph's Cypher language mainly follows the OpenCypher query standard open-sourced by Neo4j and extends auxiliary functions such as operations and maintenance management. Descriptive graph query language will become the main data operation method for graph databases, but generating the optimal execution plan between description and execution still requires a long way to go in both academia and industry."]}),"\n"]}),"\n",(0,a.jsxs)(t.blockquote,{children:["\n",(0,a.jsxs)(t.p,{children:[(0,a.jsx)(t.strong,{children:"Procedure API"})," is designed to bridge the gap between descriptive graph query language and optimal performance. TuGraph's Procedure API provides a simple wrapper on top of the Core API, which maximizes the storage's performance efficiency and is the upper limit of Cypher optimization performance. Python Procedure API is a cross-language wrapper on top of C++ Procedure API, but the copying of values during translation may result in some performance loss, and its advantage lies mainly in the ease of use of the Python language. The traversal API is a parallel-executing Procedure interface that is more similar to set operations in terms of description, such as expanding all outgoing neighbors of a point set to obtain a new point set."]}),"\n"]}),"\n",(0,a.jsxs)(t.blockquote,{children:["\n",(0,a.jsxs)(t.p,{children:[(0,a.jsx)(t.strong,{children:"OLAP API"}),' belongs to the category of "graph computing systems" and exports snapshots of graph data from storage that supports insert, update, delete, and query operations to support read-only complex graph analysis in a more compact data storage format. OLAP API encapsulates data structures that support high-concurrency execution, including Vector, Bitmap, and CSR-based graph snapshot data structures, and provides a set of concurrent fast point-edge operation frameworks. After the graph analysis task is completed, the data can be written back to the graph database through the interface.']}),"\n"]}),"\n",(0,a.jsxs)(t.blockquote,{children:["\n",(0,a.jsxs)(t.p,{children:[(0,a.jsx)(t.strong,{children:"GNN API"})," mainly provides the interfaces needed for graph neural network applications and can be integrated with machine learning frameworks such as PyTorch. TuGraph's GNN PI mainly integrates DGL and completes the entire process from graph storage to graph neural network application in the Python language environment."]}),"\n"]}),"\n",(0,a.jsx)(t.p,{children:"Except for Cypher's interpretive execution, all other server interfaces are compiled and executed, meaning that the corresponding code needs to be sent to the server and compiled (which may take some time) before execution on the server. Therefore, it is usually necessary to load the program first, then find it in the list of loaded applications, and execute it after passing in the input parameters."})]})}function h(e={}){const{wrapper:t}={...(0,r.R)(),...e.components};return t?(0,a.jsx)(t,{...e,children:(0,a.jsx)(d,{...e})}):d(e)}},85780:(e,t,n)=>{n.d(t,{A:()=>a});const a=n.p+"assets/images/multi-level-Interfaces-en-78e3e2c8e270a26703bede09a6377d7e.png"},28453:(e,t,n)=>{n.d(t,{R:()=>o,x:()=>s});var a=n(96540);const r={},i=a.createContext(r);function o(e){const t=a.useContext(i);return a.useMemo((function(){return"function"==typeof e?e(t):{...t,...e}}),[t,e])}function s(e){let t;return t=e.disableParentContext?"function"==typeof e.components?e.components(r):e.components||r:o(e.components),a.createElement(i.Provider,{value:t},e.children)}}}]);
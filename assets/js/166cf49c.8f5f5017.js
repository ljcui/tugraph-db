"use strict";(self.webpackChunkdocusaurus=self.webpackChunkdocusaurus||[]).push([[57637],{44840:(e,n,r)=>{r.r(n),r.d(n,{assets:()=>d,contentTitle:()=>i,default:()=>l,frontMatter:()=>o,metadata:()=>s,toc:()=>h});var t=r(74848),a=r(28453);const o={},i="The Three Body",s={id:"quick-start/demo/the-three-body",title:"The Three Body",description:"This document mainly introduces the usage of Three-Body demo.",source:"@site/versions/version-4.5.1/en-US/source/3.quick-start/2.demo/3.the-three-body.md",sourceDirName:"3.quick-start/2.demo",slug:"/quick-start/demo/the-three-body",permalink:"/tugraph-db/en/4.5.1/quick-start/demo/the-three-body",draft:!1,unlisted:!1,tags:[],version:"current",sidebarPosition:3,frontMatter:{},sidebar:"tutorialSidebar",previous:{title:"DEMO:Wandering Earth",permalink:"/tugraph-db/en/4.5.1/quick-start/demo/wandering-earth"},next:{title:"Three Kingdoms",permalink:"/tugraph-db/en/4.5.1/quick-start/demo/three-kingdoms"}},d={},h=[{value:"1.Demo Scenario Design",id:"1demo-scenario-design",level:2},{value:"2.Instructions for Use",id:"2instructions-for-use",level:2},{value:"3.Data Import",id:"3data-import",level:2},{value:"4.Cypher Query",id:"4cypher-query",level:2},{value:"5.Usage Demo",id:"5usage-demo",level:2},{value:"5.1.Data Import Demo",id:"51data-import-demo",level:3},{value:"6.Query Demo",id:"6query-demo",level:2},{value:"6.1.Character Relationship Query",id:"61character-relationship-query",level:3},{value:"6.2.Neighbor Vertex Analysis",id:"62neighbor-vertex-analysis",level:3},{value:"6.3.Query Common Neighbors of Node A and B",id:"63query-common-neighbors-of-node-a-and-b",level:3},{value:"6.4.Set/Change Properties",id:"64setchange-properties",level:3},{value:"6.5.Add/Delete Nodes",id:"65adddelete-nodes",level:3}];function c(e){const n={a:"a",blockquote:"blockquote",code:"code",h1:"h1",h2:"h2",h3:"h3",header:"header",img:"img",li:"li",p:"p",pre:"pre",ul:"ul",...(0,a.R)(),...e.components};return(0,t.jsxs)(t.Fragment,{children:[(0,t.jsx)(n.header,{children:(0,t.jsx)(n.h1,{id:"the-three-body",children:"The Three Body"})}),"\n",(0,t.jsxs)(n.blockquote,{children:["\n",(0,t.jsx)(n.p,{children:"This document mainly introduces the usage of Three-Body demo."}),"\n"]}),"\n",(0,t.jsx)(n.h2,{id:"1demo-scenario-design",children:"1.Demo Scenario Design"}),"\n",(0,t.jsx)(n.p,{children:"The demo scenario is designed based on the story background of Three-Body 1, 2, and 3."}),"\n",(0,t.jsx)(n.p,{children:'According to the setting of the Three-Body story, we have designed 4 types of nodes and 6 types of edges. The nodes include "character", "organization", "plan", and "timeline", while the edges include "character-character relationship", "character-plan relationship", "character-organization relationship", "organization-plan relationship", and "organization-organization relationship". According to the plot, corresponding schema data and some queries have been prepared to propose some questions about the plot.'}),"\n",(0,t.jsx)(n.h2,{id:"2instructions-for-use",children:"2.Instructions for Use"}),"\n",(0,t.jsx)(n.p,{children:"Prerequisite: TuGraph is installed."}),"\n",(0,t.jsx)(n.h2,{id:"3data-import",children:"3.Data Import"}),"\n",(0,t.jsxs)(n.ul,{children:["\n",(0,t.jsxs)(n.li,{children:["Manual import\n",(0,t.jsxs)(n.ul,{children:["\n",(0,t.jsxs)(n.li,{children:["Data storage directory: ",(0,t.jsx)(n.a,{href:"https://github.com/TuGraph-family/tugraph-db-demo",children:"https://github.com/TuGraph-family/tugraph-db-demo"}),"."]}),"\n",(0,t.jsxs)(n.li,{children:["Modify the DATA_PATH in import.json according to the corresponding data storage directory. For more details, please refer to ",(0,t.jsx)(n.a,{href:"/tugraph-db/en/4.5.1/utility-tools/data-import",children:"Data Importing"}),"."]}),"\n",(0,t.jsx)(n.li,{children:"After starting the TuGraph service, access ${HOST_IP}:7070 to open the web page and confirm whether the data is imported successfully."}),"\n"]}),"\n"]}),"\n",(0,t.jsxs)(n.li,{children:["Automatic creation\n",(0,t.jsxs)(n.ul,{children:["\n",(0,t.jsxs)(n.li,{children:["Click ",(0,t.jsx)(n.code,{children:"New Graph Project"}),", select Three-Body data, fill in the graph project configuration, and the system will automatically create the Three-Body scenario graph project."]}),"\n"]}),"\n"]}),"\n"]}),"\n",(0,t.jsx)(n.h2,{id:"4cypher-query",children:"4.Cypher Query"}),"\n",(0,t.jsx)(n.p,{children:"Refer to the TuGraph documentation and enter Cypher in the TuGraph web page frontend to execute a query."}),"\n",(0,t.jsx)(n.h2,{id:"5usage-demo",children:"5.Usage Demo"}),"\n",(0,t.jsx)(n.h3,{id:"51data-import-demo",children:"5.1.Data Import Demo"}),"\n",(0,t.jsx)(n.p,{children:(0,t.jsx)(n.img,{alt:"data",src:r(80410).A+"",width:"1527",height:"999"})}),"\n",(0,t.jsx)(n.h2,{id:"6query-demo",children:"6.Query Demo"}),"\n",(0,t.jsx)(n.h3,{id:"61character-relationship-query",children:"6.1.Character Relationship Query"}),"\n",(0,t.jsx)(n.p,{children:"In the plot of Three-Body 1, a large number of scientists committed suicide all over the world at the beginning, which aroused the attention of the police. During the investigation process, the truth behind it gradually surfaced step by step according to the clues of the character relationships, as shown in the above graph. Shi Qiang and Wang Miao discovered that most people had direct or indirect connections with Ye Wenjie, and sent Wang Miao undercover to finally discover Ye Wenjie's ultimate commander identity. As can be seen in the graph, there are many edge relationships (many one-degree or two-degree neighbors) around Ye Wenjie's vertex."}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-cypher",children:"MATCH (n)-[e:person_person]-(m) RETURN n,e,m\n"})}),"\n",(0,t.jsx)(n.p,{children:(0,t.jsx)(n.img,{alt:"data",src:r(34906).A+"",width:"1527",height:"999"})}),"\n",(0,t.jsx)(n.h3,{id:"62neighbor-vertex-analysis",children:"6.2.Neighbor Vertex Analysis"}),"\n",(0,t.jsx)(n.p,{children:'There are many plans in Three-Body, and sometimes we may be confused. At this time, we can use the graph\'s neighbor vertex query to view the relevant characters and organizations of the plan. For example, in the "Wallfacer Project", we can see that there are four characters related to it, and these four are also the "Wallfacers" who are highly expected by the world.'}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-cypher",children:'MATCH (p:plan {name: "\u9762\u58c1\u8ba1\u5212"})-[e]-(neighbor:person)\nRETURN neighbor,p,e\n'})}),"\n",(0,t.jsx)(n.p,{children:(0,t.jsx)(n.img,{alt:"data",src:r(59569).A+"",width:"1527",height:"999"})}),"\n",(0,t.jsx)(n.h3,{id:"63query-common-neighbors-of-node-a-and-b",children:"6.3.Query Common Neighbors of Node A and B"}),"\n",(0,t.jsx)(n.p,{children:"We often want to know who are the common related characters between two characters, so that we can quickly grasp the relationship between these two characters. In the case of large amounts of data, it is very convenient to use Cypher for graph relationship analysis!"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-cypher",children:'MATCH (a:person {name: "\u53f6\u6587\u6d01"})-[e1:person_person]->(n)<-[e2:person_person]-(b:person {name: "\u6c6a\u6dfc"})\nRETURN a,b,n,e1,e2\n'})}),"\n",(0,t.jsx)(n.p,{children:(0,t.jsx)(n.img,{alt:"data",src:r(3272).A+"",width:"1527",height:"999"})}),"\n",(0,t.jsx)(n.h3,{id:"64setchange-properties",children:"6.4.Set/Change Properties"}),"\n",(0,t.jsx)(n.p,{children:'As the plot progresses, we gradually learn about the various labels on "Ye Wenjie", so we can also update these labels to the "Ye Wenjie" node:'}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-cypher",children:'MATCH (p:person {name: "\u53f6\u6587\u6d01"})\nSET p.introduce = "\u6e05\u534e\u5927\u5b66\u6559\u6388\u3001ETO\u7cbe\u795e\u9886\u8896\u3001\u9996\u4f4d\u548c\u4e09\u4f53\u4eba\u4ea4\u6d41\u7684\u4eba"\nRETURN p\n'})}),"\n",(0,t.jsx)(n.h3,{id:"65adddelete-nodes",children:"6.5.Add/Delete Nodes"}),"\n",(0,t.jsx)(n.p,{children:"Later, we learned about characters such as Luo Ji and Cheng Xin, and organizations such as PIA and the Star Ring Group, and wanted to add them as nodes to the graph:"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-cypher",children:'CREATE (a:person {introduce: "\u7269\u7406\u5b66\u6559\u6388", name: "bbb"})\nRETURN a\n'})})]})}function l(e={}){const{wrapper:n}={...(0,a.R)(),...e.components};return n?(0,t.jsx)(n,{...e,children:(0,t.jsx)(c,{...e})}):c(e)}},34906:(e,n,r)=>{r.d(n,{A:()=>t});const t=r.p+"assets/images/three-body-cypher1-f48c03a7aaf7b1ee2b1dd8b302bf956c.png"},59569:(e,n,r)=>{r.d(n,{A:()=>t});const t=r.p+"assets/images/three-body-cypher2-91d2678a5e9fb47de7ae9e31fd33d264.png"},3272:(e,n,r)=>{r.d(n,{A:()=>t});const t=r.p+"assets/images/three-body-cypher3-4ce04e7d3f064d8f1b703b36de2156e5.png"},80410:(e,n,r)=>{r.d(n,{A:()=>t});const t=r.p+"assets/images/three-body-data-8e5b825e6b329f9026994a69bdbb5ead.png"},28453:(e,n,r)=>{r.d(n,{R:()=>i,x:()=>s});var t=r(96540);const a={},o=t.createContext(a);function i(e){const n=t.useContext(o);return t.useMemo((function(){return"function"==typeof e?e(n):{...n,...e}}),[n,e])}function s(e){let n;return n=e.disableParentContext?"function"==typeof e.components?e.components(a):e.components||a:i(e.components),t.createElement(o.Provider,{value:n},e.children)}}}]);
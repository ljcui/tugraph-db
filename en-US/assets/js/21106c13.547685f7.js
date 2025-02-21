"use strict";(self.webpackChunkdocusaurus=self.webpackChunkdocusaurus||[]).push([[60158],{9667:(e,t,r)=>{r.d(t,{A:()=>n});const n=r.p+"assets/images/wandering-earth-2-d6bbcf44b71e4f7e8927ac46e99f8636.png"},19834:(e,t,r)=>{r.d(t,{A:()=>n});const n=r.p+"assets/images/wandering-earth-3-79833e91472e6b0c46d7d663b964728a.png"},22912:(e,t,r)=>{r.r(t),r.d(t,{assets:()=>o,contentTitle:()=>a,default:()=>c,frontMatter:()=>s,metadata:()=>d,toc:()=>h});var n=r(74848),i=r(28453);const s={},a="DEMO Earth",d={id:"quick-start/demo/wandering-earth",title:"DEMO:Wandering Earth",description:"This document mainly introduces the usage of the Wandering Earth demo.",source:"@site/versions/version-4.5.1/en-US/source/3.quick-start/2.demo/2.wandering-earth.md",sourceDirName:"3.quick-start/2.demo",slug:"/quick-start/demo/wandering-earth",permalink:"/tugraph-db/en-US/en/4.5.1/quick-start/demo/wandering-earth",draft:!1,unlisted:!1,tags:[],version:"current",sidebarPosition:2,frontMatter:{},sidebar:"tutorialSidebar",previous:{title:"DEMO:Movie",permalink:"/tugraph-db/en-US/en/4.5.1/quick-start/demo/movie"},next:{title:"The Three Body",permalink:"/tugraph-db/en-US/en/4.5.1/quick-start/demo/the-three-body"}},o={},h=[{value:"1.Demo Scene Design",id:"1demo-scene-design",level:2},{value:"2.Instructions for Use",id:"2instructions-for-use",level:2},{value:"3.Data Import",id:"3data-import",level:2},{value:"4.Cypher Query",id:"4cypher-query",level:2},{value:"5.Usage Display",id:"5usage-display",level:2},{value:"5.1.Data Import Display",id:"51data-import-display",level:3},{value:"5.2.Query Display",id:"52query-display",level:3}];function l(e){const t={a:"a",blockquote:"blockquote",h1:"h1",h2:"h2",h3:"h3",header:"header",img:"img",li:"li",p:"p",ul:"ul",...(0,i.R)(),...e.components};return(0,n.jsxs)(n.Fragment,{children:[(0,n.jsx)(t.header,{children:(0,n.jsx)(t.h1,{id:"demo-earth",children:"DEMO:Wandering Earth"})}),"\n",(0,n.jsxs)(t.blockquote,{children:["\n",(0,n.jsx)(t.p,{children:"This document mainly introduces the usage of the Wandering Earth demo."}),"\n"]}),"\n",(0,n.jsx)(t.h2,{id:"1demo-scene-design",children:"1.Demo Scene Design"}),"\n",(0,n.jsx)(t.p,{children:"The demo is based on the story background of The Wandering Earth 1 and The Wandering Earth 2."}),"\n",(0,n.jsxs)(t.ul,{children:["\n",(0,n.jsx)(t.li,{children:"Based on the plot, a graph structure is designed, including three types of points: organization, character, celestial body, and facility, and two types of edges: event and relationship."}),"\n",(0,n.jsx)(t.li,{children:"Prepared data corresponding to the schema based on the plot."}),"\n",(0,n.jsx)(t.li,{children:"Prepared some queries to ask questions about the plot."}),"\n"]}),"\n",(0,n.jsx)(t.h2,{id:"2instructions-for-use",children:"2.Instructions for Use"}),"\n",(0,n.jsx)(t.p,{children:"Prerequisite: TuGraph is installed."}),"\n",(0,n.jsx)(t.h2,{id:"3data-import",children:"3.Data Import"}),"\n",(0,n.jsxs)(t.ul,{children:["\n",(0,n.jsxs)(t.li,{children:["Data storage directory: ",(0,n.jsx)(t.a,{href:"https://github.com/TuGraph-family/tugraph-db-demo",children:"https://github.com/TuGraph-family/tugraph-db-demo"}),"."]}),"\n",(0,n.jsxs)(t.li,{children:["Modify the DATA_PATH in import.json according to the corresponding data storage directory. For more details, please refer to ",(0,n.jsx)(t.a,{href:"/tugraph-db/en-US/en/4.5.1/utility-tools/data-import",children:"Data Importing"}),"."]}),"\n",(0,n.jsx)(t.li,{children:"After starting the TuGraph service, access ${HOST_IP}:7070 to open the web page and confirm whether the data is imported successfully."}),"\n"]}),"\n",(0,n.jsx)(t.h2,{id:"4cypher-query",children:"4.Cypher Query"}),"\n",(0,n.jsx)(t.p,{children:"Refer to the Cypher document and enter Cypher in the TuGraph web page frontend for queries."}),"\n",(0,n.jsx)(t.h2,{id:"5usage-display",children:"5.Usage Display"}),"\n",(0,n.jsx)(t.h3,{id:"51data-import-display",children:"5.1.Data Import Display"}),"\n",(0,n.jsx)(t.p,{children:(0,n.jsx)(t.img,{alt:"\u6570\u636e\u5bfc\u5165\u5c55\u793a",src:r(47688).A+"",width:"1527",height:"1120"})}),"\n",(0,n.jsx)(t.h3,{id:"52query-display",children:"5.2.Query Display"}),"\n",(0,n.jsx)(t.p,{children:"Query all event processes related to the crisis on Jupiter."}),"\n",(0,n.jsx)(t.p,{children:(0,n.jsx)(t.img,{alt:"\u6570\u636e\u5bfc\u5165\u5c55\u793a",src:r(9667).A+"",width:"1527",height:"1120"})}),"\n",(0,n.jsx)(t.p,{children:"Query all event processes related to all crises."}),"\n",(0,n.jsx)(t.p,{children:(0,n.jsx)(t.img,{alt:"\u6570\u636e\u5bfc\u5165\u5c55\u793a",src:r(19834).A+"",width:"1527",height:"1120"})})]})}function c(e={}){const{wrapper:t}={...(0,i.R)(),...e.components};return t?(0,n.jsx)(t,{...e,children:(0,n.jsx)(l,{...e})}):l(e)}},28453:(e,t,r)=>{r.d(t,{R:()=>a,x:()=>d});var n=r(96540);const i={},s=n.createContext(i);function a(e){const t=n.useContext(s);return n.useMemo((function(){return"function"==typeof e?e(t):{...t,...e}}),[t,e])}function d(e){let t;return t=e.disableParentContext?"function"==typeof e.components?e.components(i):e.components||i:a(e.components),n.createElement(s.Provider,{value:t},e.children)}},47688:(e,t,r)=>{r.d(t,{A:()=>n});const n=r.p+"assets/images/wandering-earth-1-0d55cfd964d4531decde10ca519f0435.png"}}]);
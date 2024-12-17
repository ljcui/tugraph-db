"use strict";(self.webpackChunkdocusaurus=self.webpackChunkdocusaurus||[]).push([[69557],{8576:(e,n,r)=>{r.r(n),r.d(n,{assets:()=>l,contentTitle:()=>h,default:()=>o,frontMatter:()=>i,metadata:()=>t,toc:()=>a});var s=r(74848),d=r(28453);const i={},h="\u573a\u666f\uff1a\u4e09\u4f53",t={id:"quick-start/demo/the-three-body",title:"\u573a\u666f\uff1a\u4e09\u4f53",description:"\u6b64\u6587\u6863\u4e3b\u8981\u4ecb\u7ecd \u4e09\u4f53 demo\u7684\u4f7f\u7528\u65b9\u6cd5\u3002",source:"@site/versions/version-4.5.1/zh-CN/source/3.quick-start/2.demo/3.the-three-body.md",sourceDirName:"3.quick-start/2.demo",slug:"/quick-start/demo/the-three-body",permalink:"/tugraph-db/en-US/zh/4.5.1/quick-start/demo/the-three-body",draft:!1,unlisted:!1,tags:[],version:"current",sidebarPosition:3,frontMatter:{},sidebar:"tutorialSidebar",previous:{title:"\u573a\u666f\uff1a\u6d41\u6d6a\u5730\u7403",permalink:"/tugraph-db/en-US/zh/4.5.1/quick-start/demo/wandering-earth"},next:{title:"\u573a\u666f\uff1a\u4e09\u56fd",permalink:"/tugraph-db/en-US/zh/4.5.1/quick-start/demo/three-kingdoms"}},l={},a=[{value:"1.Demo\u573a\u666f\u8bbe\u8ba1",id:"1demo\u573a\u666f\u8bbe\u8ba1",level:2},{value:"2.\u4f7f\u7528\u8bf4\u660e",id:"2\u4f7f\u7528\u8bf4\u660e",level:2},{value:"3.\u6570\u636e\u5bfc\u5165",id:"3\u6570\u636e\u5bfc\u5165",level:2},{value:"4.Cypher\u67e5\u8be2",id:"4cypher\u67e5\u8be2",level:2},{value:"5.\u4f7f\u7528\u5c55\u793a",id:"5\u4f7f\u7528\u5c55\u793a",level:2},{value:"5.1.\u6570\u636e\u5bfc\u5165\u7684\u5c55\u793a",id:"51\u6570\u636e\u5bfc\u5165\u7684\u5c55\u793a",level:3},{value:"6.\u67e5\u8be2\u5c55\u793a",id:"6\u67e5\u8be2\u5c55\u793a",level:2},{value:"6.1.\u4eba\u7269\u5173\u7cfb\u67e5\u8be2",id:"61\u4eba\u7269\u5173\u7cfb\u67e5\u8be2",level:3},{value:"6.2.\u90bb\u57df\u9876\u70b9\u5206\u6790",id:"62\u90bb\u57df\u9876\u70b9\u5206\u6790",level:3},{value:"6.3.\u67e5\u8be2a\u8282\u70b9\u548cb\u8282\u70b9\u7684\u5171\u540c\u90bb\u5c45",id:"63\u67e5\u8be2a\u8282\u70b9\u548cb\u8282\u70b9\u7684\u5171\u540c\u90bb\u5c45",level:3},{value:"6.4.\u8bbe\u7f6e/\u66f4\u6539\u5c5e\u6027",id:"64\u8bbe\u7f6e\u66f4\u6539\u5c5e\u6027",level:3},{value:"6.5.\u589e\u52a0/\u5220\u9664\u8282\u70b9",id:"65\u589e\u52a0\u5220\u9664\u8282\u70b9",level:3}];function c(e){const n={a:"a",blockquote:"blockquote",code:"code",h1:"h1",h2:"h2",h3:"h3",header:"header",img:"img",li:"li",p:"p",pre:"pre",ul:"ul",...(0,d.R)(),...e.components};return(0,s.jsxs)(s.Fragment,{children:[(0,s.jsx)(n.header,{children:(0,s.jsx)(n.h1,{id:"\u573a\u666f\u4e09\u4f53",children:"\u573a\u666f\uff1a\u4e09\u4f53"})}),"\n",(0,s.jsxs)(n.blockquote,{children:["\n",(0,s.jsx)(n.p,{children:"\u6b64\u6587\u6863\u4e3b\u8981\u4ecb\u7ecd \u4e09\u4f53 demo\u7684\u4f7f\u7528\u65b9\u6cd5\u3002"}),"\n"]}),"\n",(0,s.jsx)(n.h2,{id:"1demo\u573a\u666f\u8bbe\u8ba1",children:"1.Demo\u573a\u666f\u8bbe\u8ba1"}),"\n",(0,s.jsx)(n.p,{children:"Demo\u80cc\u666f\u57fa\u4e8e\u4e09\u4f531\u3001\u4e09\u4f532\u3001\u4e09\u4f533\u7684\u6545\u4e8b\u80cc\u666f\u8fdb\u884c\u8bbe\u8ba1\u3002"}),"\n",(0,s.jsx)(n.p,{children:"\u6839\u636e\u4e09\u4f53\u6545\u4e8b\u7684\u8bbe\u5b9a\uff0c\u6211\u4eec\u8bbe\u8ba1\u4e864\u7c7b\u70b9\u548c6\u7c7b\u8fb9\uff0c\u70b9\u5305\u62ec\u201c\u4eba\u7269\u201d\u3001\u201c\u7ec4\u7ec7\u201d\u3001\u201c\u8ba1\u5212\u201d\u3001\u201c\u65f6\u95f4\u7ebf\u201d\uff0c\u8fb9\u5305\u62ec\u201c\u4eba\u7269-\u4eba\u7269\u5173\u7cfb\u201d\u3001\u201c\u4eba\u7269-\u8ba1\u5212\u5173\u7cfb\u201d\u3001\u201c\u4eba\u7269-\u7ec4\u7ec7\u5173\u7cfb\u201d\u3001\u201c\u7ec4\u7ec7-\u8ba1\u5212\u5173\u7cfb\u201d\u3001\u201c\u7ec4\u7ec7-\u7ec4\u7ec7\u5173\u7cfb\u201d\u3002\n\u6839\u636e\u5267\u60c5\u51c6\u5907\u4e86\u5bf9\u5e94Schema\u7684\u6570\u636e\uff0c\u51c6\u5907\u4e86\u4e00\u4e9bquery\uff0c\u63d0\u51fa\u4e00\u4e9b\u5173\u4e8e\u5267\u60c5\u7684\u95ee\u9898\u3002"}),"\n",(0,s.jsx)(n.h2,{id:"2\u4f7f\u7528\u8bf4\u660e",children:"2.\u4f7f\u7528\u8bf4\u660e"}),"\n",(0,s.jsx)(n.p,{children:"\u524d\u7f6e\u6761\u4ef6\uff1aTuGraph\u5df2\u5b89\u88c5\u3002"}),"\n",(0,s.jsx)(n.h2,{id:"3\u6570\u636e\u5bfc\u5165",children:"3.\u6570\u636e\u5bfc\u5165"}),"\n",(0,s.jsxs)(n.ul,{children:["\n",(0,s.jsxs)(n.li,{children:["\u624b\u52a8\u5bfc\u5165\n",(0,s.jsxs)(n.ul,{children:["\n",(0,s.jsxs)(n.li,{children:["\u6570\u636e\u5b58\u653e\u76ee\u5f55\uff1a",(0,s.jsx)(n.a,{href:"https://github.com/TuGraph-family/tugraph-db-demo",children:"https://github.com/TuGraph-family/tugraph-db-demo"})]}),"\n",(0,s.jsxs)(n.li,{children:["\u6839\u636e\u6570\u636e\u5b58\u653e\u76ee\u5f55\u5bf9\u5e94\u4fee\u6539import.json\u91cc\u9762\u7684DATA_PATH\u3002\u5177\u4f53\u53ef\u4ee5\u53c2\u8003",(0,s.jsx)(n.a,{href:"/tugraph-db/en-US/zh/4.5.1/utility-tools/data-import",children:"\u6570\u636e\u5bfc\u5165"})]}),"\n",(0,s.jsx)(n.li,{children:"\u542f\u52a8TuGraph\u670d\u52a1\u540e\uff0c\u8bbf\u95ee${HOST_IP}:7070\uff0c\u6253\u5f00web\u9875\u9762\uff0c\u786e\u8ba4\u6570\u636e\u662f\u5426\u5bfc\u5165\u6210\u529f\u3002"}),"\n"]}),"\n"]}),"\n",(0,s.jsxs)(n.li,{children:["\u81ea\u52a8\u521b\u5efa\n",(0,s.jsxs)(n.ul,{children:["\n",(0,s.jsxs)(n.li,{children:["\u70b9\u51fb",(0,s.jsx)(n.code,{children:"\u65b0\u5efa\u56fe\u9879\u76ee"}),"\uff0c\u9009\u62e9\u4e09\u4f53\u6570\u636e\uff0c\u586b\u5199\u56fe\u9879\u76ee\u914d\u7f6e\uff0c\u7cfb\u7edf\u4f1a\u81ea\u52a8\u5b8c\u6210\u4e09\u4f53\u573a\u666f\u56fe\u9879\u76ee\u521b\u5efa\u3002"]}),"\n"]}),"\n"]}),"\n"]}),"\n",(0,s.jsx)(n.h2,{id:"4cypher\u67e5\u8be2",children:"4.Cypher\u67e5\u8be2"}),"\n",(0,s.jsx)(n.p,{children:"\u53c2\u8003TuGraph\u6587\u6863\uff0c\u5728TuGraph\u7684Web\u9875\u9762\u524d\u7aef\u8f93\u5165Cypher\u8fdb\u884c\u67e5\u8be2\u3002"}),"\n",(0,s.jsx)(n.h2,{id:"5\u4f7f\u7528\u5c55\u793a",children:"5.\u4f7f\u7528\u5c55\u793a"}),"\n",(0,s.jsx)(n.h3,{id:"51\u6570\u636e\u5bfc\u5165\u7684\u5c55\u793a",children:"5.1.\u6570\u636e\u5bfc\u5165\u7684\u5c55\u793a"}),"\n",(0,s.jsx)(n.p,{children:(0,s.jsx)(n.img,{alt:"data",src:r(80410).A+"",width:"1527",height:"999"})}),"\n",(0,s.jsx)(n.h2,{id:"6\u67e5\u8be2\u5c55\u793a",children:"6.\u67e5\u8be2\u5c55\u793a"}),"\n",(0,s.jsx)(n.h3,{id:"61\u4eba\u7269\u5173\u7cfb\u67e5\u8be2",children:"6.1.\u4eba\u7269\u5173\u7cfb\u67e5\u8be2"}),"\n",(0,s.jsx)(n.p,{children:"\u4e09\u4f53\u4e00\u5267\u60c5\u4e2d\uff0c\u4e00\u5f00\u59cb\u5168\u4e16\u754c\u5404\u5730\u53d1\u751f\u4e86\u5927\u91cf\u79d1\u5b66\u5bb6\u81ea\u6740\u4e8b\u4ef6\uff0c\u5f15\u8d77\u4e86\u8b66\u65b9\u91cd\u89c6\uff0c\u67e5\u6848\u8fc7\u7a0b\u6839\u636e\u4eba\u7269\u5173\u7cfb\u7ebf\u7d22\u4e00\u6b65\u6b65\u6392\u67e5\uff0c\u5c31\u5982\u4e0a\u9762\u7684\u56fe\u6240\u793a\uff0c\u968f\u7740\u7ebf\u7d22\u8d8a\u6765\u8d8a\u591a\uff0c\u80cc\u540e\u7684\u771f\u76f8\u9010\u6b65\u6d6e\u51fa\u6c34\u9762\u3002\u53f2\u5f3a\u548c\u6c6a\u6dfc\u53d1\u73b0\u5927\u591a\u6570\u4eba\u90fd\u548c\u53f6\u6587\u6d01\u6709\u7740\u76f4\u63a5\u6216\u8005\u95f4\u63a5\u7684\u8054\u7cfb\uff0c\u5e76\u6d3e\u6c6a\u6dfc\u5367\u5e95\uff0c\u6700\u7ec8\u53d1\u73b0\u53f6\u6587\u6d01\u7684\u6700\u7ec8\u7edf\u5e05\u8eab\u4efd\u3002\u5728\u56fe\u4e2d\u4e5f\u53ef\u4ee5\u770b\u51fa\uff0c\u53f6\u6587\u6d01\u9876\u70b9\u5468\u56f4\u6709\u5f88\u591a\u8fb9\u5173\u7cfb\uff08\u4e00\u5ea6\u6216\u4e8c\u5ea6\u90bb\u5c45\u5f88\u591a\uff09\u3002"}),"\n",(0,s.jsx)(n.pre,{children:(0,s.jsx)(n.code,{className:"language-cypher",children:"MATCH (n)-[e:person_person]-(m) RETURN n,e,m\n"})}),"\n",(0,s.jsx)(n.p,{children:(0,s.jsx)(n.img,{alt:"data",src:r(34906).A+"",width:"1527",height:"999"})}),"\n",(0,s.jsx)(n.h3,{id:"62\u90bb\u57df\u9876\u70b9\u5206\u6790",children:"6.2.\u90bb\u57df\u9876\u70b9\u5206\u6790"}),"\n",(0,s.jsx)(n.p,{children:'\u4e09\u4f53\u4e2d\u7684\u5404\u79cd\u8ba1\u5212\u6bd4\u8f83\u591a\uff0c\u6709\u7684\u65f6\u5019\u53ef\u80fd\u4f1a\u88ab\u7ed5\u6655\uff0c\u8fd9\u65f6\u5019\u6211\u4eec\u53ef\u4ee5\u901a\u8fc7\u56fe\u7684\u90bb\u5c45\u9876\u70b9\u67e5\u8be2\u6765\u67e5\u770b\u8be5\u8ba1\u5212\u7684\u76f8\u5173\u4eba\u7269\u548c\u7ec4\u7ec7\u7b49\u3002\u5982"\u9762\u58c1\u8ba1\u5212"\u4e2d,\u6211\u4eec\u53ef\u4ee5\u770b\u5230\u6709\u56db\u4f4d\u4eba\u7269\u4e0e\u4e4b\u76f8\u5173\uff0c\u8fd9\u56db\u4f4d\u4e5f\u662f\u88ab\u4e16\u4eba\u6240\u5bc4\u4e88\u539a\u671b\u7684\u201c\u9762\u58c1\u8005\u201d\u3002'}),"\n",(0,s.jsx)(n.pre,{children:(0,s.jsx)(n.code,{className:"language-cypher",children:'MATCH (p:plan {name: "\u9762\u58c1\u8ba1\u5212"})-[e]-(neighbor:person)\nRETURN neighbor,p,e\n'})}),"\n",(0,s.jsx)(n.p,{children:(0,s.jsx)(n.img,{alt:"data",src:r(59569).A+"",width:"1527",height:"999"})}),"\n",(0,s.jsx)(n.h3,{id:"63\u67e5\u8be2a\u8282\u70b9\u548cb\u8282\u70b9\u7684\u5171\u540c\u90bb\u5c45",children:"6.3.\u67e5\u8be2a\u8282\u70b9\u548cb\u8282\u70b9\u7684\u5171\u540c\u90bb\u5c45"}),"\n",(0,s.jsx)(n.p,{children:"\u6211\u4eec\u5f80\u5f80\u5e0c\u671b\u77e5\u9053\u4e24\u4e2a\u4eba\u7269\u4e4b\u95f4\u7684\u5171\u540c\u5173\u8054\u7684\u4eba\u7269\u90fd\u6709\u8c01\uff0c\u8fd9\u6837\u5c31\u80fd\u5f88\u5feb\u7684\u638c\u63e1\u8fd9\u4e24\u4e2a\u4eba\u7269\u4e4b\u95f4\u7684\u5173\u7cfb\uff0c\u5728\u5927\u6570\u636e\u91cf\u7684\u60c5\u51b5\u4e0b\uff0c\u4f7f\u7528cypher\u8fdb\u884c\u56fe\u5173\u7cfb\u5206\u6790\u5c31\u5f88\u65b9\u4fbf\uff01"}),"\n",(0,s.jsx)(n.pre,{children:(0,s.jsx)(n.code,{className:"language-cypher",children:'MATCH (a:person {name: "\u53f6\u6587\u6d01"})-[e1:person_person]->(n)<-[e2:person_person]-(b:person {name: "\u6c6a\u6dfc"})\nRETURN a,b,n,e1,e2\n'})}),"\n",(0,s.jsx)(n.p,{children:(0,s.jsx)(n.img,{alt:"data",src:r(3272).A+"",width:"1527",height:"999"})}),"\n",(0,s.jsx)(n.h3,{id:"64\u8bbe\u7f6e\u66f4\u6539\u5c5e\u6027",children:"6.4.\u8bbe\u7f6e/\u66f4\u6539\u5c5e\u6027"}),"\n",(0,s.jsx)(n.p,{children:'\u968f\u7740\u5267\u60c5\u63a8\u8fdb\uff0c\u6211\u4eec\u9010\u6b65\u4e86\u89e3\u4e86"\u53f6\u6587\u6d01"\u8eab\u4e0a\u7684\u591a\u4e2a\u6807\u7b7e\uff0c\u90a3\u4e48\u6211\u4eec\u4e5f\u53ef\u4ee5\u5c06\u8fd9\u4e9b\u6807\u7b7e\u66f4\u65b0\u81f3\u201c\u53f6\u6587\u6d01\u201d\u8282\u70b9\u4e0a\uff1a'}),"\n",(0,s.jsx)(n.pre,{children:(0,s.jsx)(n.code,{className:"language-cypher",children:'MATCH (p:person {name: "\u53f6\u6587\u6d01"})\nSET p.introduce = "\u6e05\u534e\u5927\u5b66\u6559\u6388\u3001ETO\u7cbe\u795e\u9886\u8896\u3001\u9996\u4f4d\u548c\u4e09\u4f53\u4eba\u4ea4\u6d41\u7684\u4eba"\nRETURN p\n'})}),"\n",(0,s.jsx)(n.h3,{id:"65\u589e\u52a0\u5220\u9664\u8282\u70b9",children:"6.5.\u589e\u52a0/\u5220\u9664\u8282\u70b9"}),"\n",(0,s.jsx)(n.p,{children:"\u540e\u7eed\u6211\u4eec\u4e86\u89e3\u5230\u4e86\u7f57\u8f91\u3001\u7a0b\u5fc3\u7b49\u7b49\u4eba\u7269\u548cPIA\u3001\u661f\u73af\u96c6\u56e2\u7b49\u7ec4\u7ec7\uff0c\u5e0c\u671b\u628a\u8fd9\u4e9b\u4f5c\u4e3a\u8282\u70b9\u6dfb\u52a0\u81f3\u56fe\u4e2d\uff1a"}),"\n",(0,s.jsx)(n.pre,{children:(0,s.jsx)(n.code,{className:"language-cypher",children:'CREATE (a:person {introduce: "\u7269\u7406\u5b66\u6559\u6388", name: "bbb"})\nRETURN a\n'})})]})}function o(e={}){const{wrapper:n}={...(0,d.R)(),...e.components};return n?(0,s.jsx)(n,{...e,children:(0,s.jsx)(c,{...e})}):c(e)}},34906:(e,n,r)=>{r.d(n,{A:()=>s});const s=r.p+"assets/images/three-body-cypher1-f48c03a7aaf7b1ee2b1dd8b302bf956c.png"},59569:(e,n,r)=>{r.d(n,{A:()=>s});const s=r.p+"assets/images/three-body-cypher2-91d2678a5e9fb47de7ae9e31fd33d264.png"},3272:(e,n,r)=>{r.d(n,{A:()=>s});const s=r.p+"assets/images/three-body-cypher3-4ce04e7d3f064d8f1b703b36de2156e5.png"},80410:(e,n,r)=>{r.d(n,{A:()=>s});const s=r.p+"assets/images/three-body-data-8e5b825e6b329f9026994a69bdbb5ead.png"},28453:(e,n,r)=>{r.d(n,{R:()=>h,x:()=>t});var s=r(96540);const d={},i=s.createContext(d);function h(e){const n=s.useContext(i);return s.useMemo((function(){return"function"==typeof e?e(n):{...n,...e}}),[n,e])}function t(e){let n;return n=e.disableParentContext?"function"==typeof e.components?e.components(d):e.components||d:h(e.components),s.createElement(i.Provider,{value:n},e.children)}}}]);
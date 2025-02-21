"use strict";(self.webpackChunkdocusaurus=self.webpackChunkdocusaurus||[]).push([[22874],{17902:(n,e,i)=>{i.d(e,{A:()=>r});const r=i.p+"assets/images/htap-34a496b796d62cd33955879a7e1781c3.png"},28453:(n,e,i)=>{i.d(e,{R:()=>s,x:()=>l});var r=i(96540);const t={},c=r.createContext(t);function s(n){const e=r.useContext(c);return r.useMemo((function(){return"function"==typeof n?n(e):{...e,...n}}),[e,n])}function l(n){let e;return e=n.disableParentContext?"function"==typeof n.components?n.components(t):n.components||t:s(n.components),r.createElement(c.Provider,{value:e},n.children)}},82032:(n,e,i)=>{i.r(e),i.d(e,{assets:()=>a,contentTitle:()=>s,default:()=>o,frontMatter:()=>c,metadata:()=>l,toc:()=>d});var r=i(74848),t=i(28453);const c={},s="HTAP",l={id:"introduction/characteristics/htap",title:"HTAP",description:"\u6b64\u6587\u6863\u4e3b\u8981\u4ecb\u7ecd TuGraph \u7684HTAP\u7684\u8bbe\u8ba1\u7406\u5ff5\u3002",source:"@site/versions/version-4.3.0/zh-CN/source/2.introduction/5.characteristics/3.htap.md",sourceDirName:"2.introduction/5.characteristics",slug:"/introduction/characteristics/htap",permalink:"/tugraph-db/en-US/zh/4.3.0/introduction/characteristics/htap",draft:!1,unlisted:!1,tags:[],version:"current",sidebarPosition:3,frontMatter:{},sidebar:"tutorialSidebar",previous:{title:"\u591a\u5c42\u7ea7\u63a5\u53e3",permalink:"/tugraph-db/en-US/zh/4.3.0/introduction/characteristics/multi-level-Interfaces"},next:{title:"TuGraph\u4ea7\u54c1\u67b6\u6784",permalink:"/tugraph-db/en-US/zh/4.3.0/introduction/architecture"}},a={},d=[{value:"1.\u7b80\u4ecb",id:"1\u7b80\u4ecb",level:2},{value:"2.\u8bbe\u8ba1",id:"2\u8bbe\u8ba1",level:2}];function h(n){const e={blockquote:"blockquote",h1:"h1",h2:"h2",header:"header",img:"img",li:"li",p:"p",ul:"ul",...(0,t.R)(),...n.components};return(0,r.jsxs)(r.Fragment,{children:[(0,r.jsx)(e.header,{children:(0,r.jsx)(e.h1,{id:"htap",children:"HTAP"})}),"\n",(0,r.jsxs)(e.blockquote,{children:["\n",(0,r.jsx)(e.p,{children:"\u6b64\u6587\u6863\u4e3b\u8981\u4ecb\u7ecd TuGraph \u7684HTAP\u7684\u8bbe\u8ba1\u7406\u5ff5\u3002"}),"\n"]}),"\n",(0,r.jsx)(e.h2,{id:"1\u7b80\u4ecb",children:"1.\u7b80\u4ecb"}),"\n",(0,r.jsx)(e.p,{children:(0,r.jsx)(e.img,{alt:"htap\u67b6\u6784",src:i(17902).A+"",width:"940",height:"554"})}),"\n",(0,r.jsx)(e.p,{children:"HTAP \u7684\u5b9e\u73b0\u5728\u67b6\u6784\u4e0a\u6709\u591a\u79cd\u4e0d\u540c\u7684\u65b9\u5f0f\uff1a\u4e00\u662f\u7528\u4e24\u4e2a\u4e0d\u540c\u7684\u526f\u672c\u6765\u5206\u522b\u5904\u7406 OLTP \u548c OLAP \u7684\u4efb\u52a1\uff0c\u6838\u5fc3\u662f\u6570\u636e\u7684\u4e00\u81f4\u6027\u540c\u6b65\u548c\u989d\u5916\u7684\u8d44\u6e90\u5f00\u9500\uff1b\u4e8c\u662f\u5728\u4efb\u4f55\u65f6\u5019\u90fd\u4f7f\u7528\u540c\u4e00\u4efd\u6570\u636e\u5b58\u50a8\uff0c\u8be5\u5b9e\u73b0\u7684\u6570\u636e\u7ed3\u6784\u5b58\u5728\u5185\u5b58\u81a8\u80c0\uff0c\u5728\u5de5\u4e1a\u5316\u843d\u5730\u8fd8\u9700\u8981\u8fdb\u4e00\u6b65\u5de5\u4f5c\u3002\u5728 TuGraph \u7684\u8bbe\u8ba1\u4e2d\uff0c\u7b80\u5355\u7684 OLAP \u64cd\u4f5c\u548c OLTP \u64cd\u4f5c\u5171\u7528\u4e00\u4efd\u6570\u636e\uff0c\u800c\u590d\u6742\u7684 OLAP \u64cd\u4f5c\u5219\u5355\u72ec\u5bfc\u51fa\u5feb\u7167\u5904\u7406\u3002"}),"\n",(0,r.jsx)(e.h2,{id:"2\u8bbe\u8ba1",children:"2.\u8bbe\u8ba1"}),"\n",(0,r.jsx)(e.p,{children:"\u5728 TuGraph \u4e2d\uff0cOLTP \u4e3a\u56fe\u4e8b\u52a1\u5f15\u64ce\uff0c\u5728\u56fe 4.4\u5bf9\u5e94\u4e8b\u52a1\u64cd\u4f5c\uff1bOLAP \u4e3a\u56fe\u5206\u6790\u5f15\u64ce\uff0c\u5bf9\u5e94\u7b80\u5355\u56fe\u5206\u6790\u64cd\u4f5c\uff08\u6bd4\u5982 SPSP\uff09\u548c\u590d\u6742\u56fe\u5206\u6790\u64cd\u4f5c\uff08\u6bd4\u5982 PageRank\uff09\uff0c\u524d\u8005\u53ef\u4ee5\u76f4\u63a5\u5728\u56fe\u5b58\u50a8\u4e0a\u6267\u884c\uff0c\u800c\u540e\u8005\u9700\u8981\u989d\u5916\u5bfc\u51fa\u5feb\u7167\u6267\u884c\u3002"}),"\n",(0,r.jsxs)(e.ul,{children:["\n",(0,r.jsx)(e.li,{children:"\u4e8b\u52a1\u64cd\u4f5c\uff0c\u5373\u56fe\u4e8b\u52a1\u5f15\u64ce\u6d4b\u7684\u64cd\u4f5c\uff0c\u4e3a\u5c40\u90e8\u56fe\u7684\u589e\u5220\u67e5\u6539\u64cd\u4f5c\uff0c\u5178\u578b\u7684\u5e94\u7528\u4e3a K \u8df3\u8bbf\u95ee K-Hop\u3002"}),"\n",(0,r.jsx)(e.li,{children:"\u7b80\u5355\u5206\u6790\u64cd\u4f5c\uff0c\u662f\u56fe\u5206\u6790\u5f15\u64ce\u4e2d\u8f83\u4e3a\u7b80\u5355\u7684\u90e8\u5206\uff0c\u901a\u5e38\u4e5f\u662f\u5c40\u90e8\u7684\u56fe\u5206\u6790\u64cd\u4f5c\uff0c\u6bd4\u5982\u4e24\u70b9\u95f4\u6700\u77ed\u8def\u7b97\u6cd5 SPSP\u3001Jaccard \u7b97\u6cd5\u3002"}),"\n",(0,r.jsx)(e.li,{children:"\u590d\u6742\u5206\u6790\u64cd\u4f5c\uff0c\u662f\u56fe\u5206\u6790\u5f15\u64ce\u4e2d\u8f83\u4e3a\u590d\u6742\u7684\u90e8\u5206\uff0c\u901a\u5e38\u6d89\u53ca\u5168\u56fe\u7684\u591a\u8f6e\u6570\u636e\u8fed\u4ee3\u64cd\u4f5c\uff0c\u6bd4\u5982\u7f51\u9875\u6392\u5e8f\u7b97\u6cd5 PageRank\u3001\u793e\u533a\u53d1\u73b0\u7b97\u6cd5 Louvain\u3002"}),"\n"]}),"\n",(0,r.jsx)(e.p,{children:"\u5982\u67b6\u6784\u56fe\u6240\u793a\uff0c\u6211\u4eec\u5728\u56fe\u4e2d\u589e\u52a0\u4e86\u5916\u90e8\u5b58\u50a8\uff0c\u4f7f\u5f97\u56fe\u5206\u6790\u7684\u6570\u636e\u6e90\u4e0d\u5c40\u9650\u5728\u56fe\u6570\u636e\u5e93\u4e2d\uff0c\u53ef\u4ee5\u76f4\u63a5\u4ece\u6587\u672c\u6587\u4ef6\u8bfb\u53d6\u3002"}),"\n",(0,r.jsxs)(e.ul,{children:["\n",(0,r.jsx)(e.li,{children:"\u56fe\u5b58\u50a8\uff0c\u5373\u56fe\u6570\u636e\u5e93\u4e2d\u7684\u5b58\u50a8\uff0c\u6709\u7cbe\u5fc3\u8bbe\u8ba1\u7684\u6570\u636e\u7ed3\u6784\uff0c\u80fd\u591f\u5b8c\u6210\u5b9e\u65f6\u589e\u5220\u67e5\u6539\u3002"}),"\n",(0,r.jsx)(e.li,{children:"\u5916\u90e8\u5b58\u50a8\uff0c\u53ef\u4ee5\u662f RDBMS \u6216\u6587\u672c\u6587\u4ef6\uff0c\u4ee5\u8fb9\u8868\u7684\u7b80\u5355\u65b9\u5f0f\u5b58\u50a8\uff0c\u4ec5\u4f9b\u4e00\u6b21\u6027\u6279\u91cf\u8bfb\u53d6\uff0c\u548c\u6279\u91cf\u7ed3\u679c\u5199\u5165\u3002\u5728\u8ba1\u7b97\u5c42\uff0c\u548c\u6574\u4f53\u67b6\u6784\u56fe\u4e2d\u7684\u63a5\u53e3\u5bf9\u5e94\u3002"}),"\n",(0,r.jsx)(e.li,{children:"Cypher\uff0c\u63cf\u8ff0\u5f0f\u56fe\u67e5\u8be2\u8bed\u8a00\uff0c\u53ef\u4ee5\u5e76\u53d1\u6267\u884c\u3002"}),"\n",(0,r.jsx)(e.li,{children:"Procedure API\uff0c\u8fc7\u7a0b\u5f0f\u56fe\u67e5\u8be2\u8bed\u8a00\uff0c\u5176\u7075\u6d3b\u6027\u80fd\u591f\u540c\u65f6\u652f\u6301\u4e8b\u52a1\u64cd\u4f5c\u548c\u56fe\u5206\u6790\u64cd\u4f5c\uff0c\u4f46\u6548\u7387\u4e0a\u4e0d\u8db3\u4ee5\u5b8c\u6210\u590d\u6742\u56fe\u5206\u6790\u64cd\u4f5c\uff0c\u53ef\u4ee5\u5e76\u53d1\u6267\u884c\u3002"}),"\n",(0,r.jsx)(e.li,{children:"OLAP API\uff0c\u9488\u5bf9\u591a\u8f6e\u8fed\u4ee3\u7684\u590d\u6742\u56fe\u5206\u6790\u3002\u5e94\u7528\u9700\u8981\u5148\u5c06\u5b58\u50a8\u4e2d\u7684\u56fe\u6570\u636e\u5bfc\u51fa\u6210\u5185\u5b58\u4e2d\u7684\u4e00\u4e2a\u5feb\u7167\uff0c\u8be5\u5feb\u7167\u4ec5\u7528\u6765\u5feb\u901f\u8bbf\u95ee\uff0c\u800c\u4e0d\u9700\u8981\u8003\u8651 ACID \u7684\u5199\u652f\u6301\uff0c\u56e0\u6b64\u53ef\u4ee5\u6392\u5e03\u5730\u66f4\u52a0\u7d27\u51d1\uff0cCSR \u6392\u5e03\u7684\u8bfb\u6548\u7387\u8981\u8fdc\u9ad8\u4e8e\u56fe\u5b58\u50a8\u7684\u6570\u636e\u6392\u5e03\u3002OLAP API \u53ea\u80fd\u4e32\u884c\u6267\u884c\uff0c\u6bcf\u4e2a\u64cd\u4f5c\u90fd\u7528\u6ee1 CPU \u8d44\u6e90\u3002"}),"\n"]}),"\n",(0,r.jsx)(e.p,{children:"OLAP API \u7684\u5feb\u7167\u53ef\u4ee5\u4ece\u5916\u90e8\u5b58\u50a8\u521b\u5efa\uff0c\u5373\u5c06\u8fb9\u8868\u6570\u636e\u6784\u6210\u4ef6 CSR \u7684\u683c\u5f0f\uff1b\u6216\u8005\u4ece\u56fe\u5b58\u50a8\u4e2d\u521b\u5efa\u3002\u9700\u8981\u6ce8\u610f\u7684\u662f\uff0cOLAP API \u8981\u6c42\u70b9\u7684 ID \u662f\u8fde\u7eed\u7684\u81ea\u7136\u6570\uff0c\u53ef\u80fd\u9700\u8981\u989d\u5916\u7684 ID \u6620\u5c04\uff0c\u8be5\u6b65\u9aa4\u5728\u521b\u5efa\u5feb\u7167\u65f6\u53ef\u4ee5\u5728\u6307\u5b9a\u4e00\u4e2a\u5c5e\u6027\u8fdb\u884c\u6620\u5c04\uff0c\u6216\u76f4\u63a5\u53d6\u5c5e\u6027\u503c\u4f5c\u4e3a ID\u3002"}),"\n",(0,r.jsx)(e.p,{children:"\u4e0e\u8ba1\u7b97\u63a5\u53e3\u548c\u5b58\u50a8\u76f8\u5bf9\u5e94\uff0c\u6709\u56db\u79cd\u8fd0\u884c\u6a21\u5f0f\u3002"}),"\n",(0,r.jsxs)(e.ul,{children:["\n",(0,r.jsx)(e.li,{children:"\u4e8b\u52a1\u6a21\u5f0f\uff0c\u6bcf\u4e2a\u64cd\u4f5c\u5bf9\u5e94\u4e00\u6761 Cypher \u8bed\u53e5\uff0c\u9ed8\u8ba4\u662f\u4e00\u4e2a\u4e8b\u52a1\u3002"}),"\n",(0,r.jsx)(e.li,{children:"Plugin \u6a21\u5f0f\uff0c\u901a\u8fc7\u63d2\u4ef6\u7684\u65b9\u5f0f\uff0c\u5728\u8ba1\u7b97\u903b\u8f91\u52a0\u8f7d\u5230\u670d\u52a1\u7aef\u540e\u8c03\u7528\uff0c\u4e5f\u53eb\u5b58\u50a8\u8fc7\u7a0b\u3002"}),"\n",(0,r.jsx)(e.li,{children:"Embed \u6a21\u5f0f\uff0c\u548c Plugin \u6a21\u5f0f\u7684\u4f7f\u7528\u63a5\u53e3\u4e00\u81f4\uff0c\u533a\u522b\u662f\u56fe\u6570\u636e\u5e93\u670d\u52a1\u4e0d\u9700\u8981\u8d77\u6765\uff0c\u5c31\u53ef\u4ee5\u76f4\u63a5\u7528\u63a5\u53e3\u8c03\u7528\u6570\u636e\u5e93\u4e2d\u7684\u6570\u636e\uff0c\u901a\u5e38\u7528\u4e8e\u8c03\u8bd5 Procedure API \u548cOLAP API \u7684\u4ee3\u7801\uff0c\u8c03\u8bd5\u4fe1\u606f\u548c\u64cd\u4f5c\u6b65\u9aa4\u6bd4 Plugin \u6a21\u5f0f\u66f4\u52a0\u53cb\u597d\u3002"}),"\n",(0,r.jsx)(e.li,{children:"Standalone \u6a21\u5f0f\uff0c\u6700\u5927\u7a0b\u5ea6\u5265\u79bb\u4e0e\u56fe\u6570\u636e\u5e93\u7684\u5173\u7cfb\uff0c\u4ec5\u60f3\u7528\u56fe\u5206\u6790\u5f15\u64ce\u505a\u6570\u636e\u5206\u6790\u65f6\uff0c\u8be5\u6a21\u5f0f\u4f1a\u6bd4\u8f83\u76f4\u63a5\u3002Standalone \u6a21\u5f0f\u4f1a\u76f4\u63a5\u4f7f\u7528\u5916\u90e8\u5b58\u50a8\u7684\u6570\u636e\u3002"}),"\n"]}),"\n",(0,r.jsx)(e.p,{children:"\u56fe\u795e\u7ecf\u7f51\u7edc\u5f15\u64ce\u7684\u4f7f\u7528\u65b9\u5f0f\u548c \u2018\u590d\u6742\u56fe\u5206\u6790\u64cd\u4f5c\u2019 \u7c7b\u4f3c\uff0c\u4f1a\u540c\u65f6\u8c03\u7528\u90e8\u5206 OLAP API \u548c GNN API\uff0c\u4e0d\u5728\u8fd9\u91cc\u5c55\u5f00\u3002"})]})}function o(n={}){const{wrapper:e}={...(0,t.R)(),...n.components};return e?(0,r.jsx)(e,{...n,children:(0,r.jsx)(h,{...n})}):h(n)}}}]);
"use strict";(self.webpackChunkdocusaurus=self.webpackChunkdocusaurus||[]).push([[67413],{68907:(e,n,r)=>{r.r(n),r.d(n,{assets:()=>t,contentTitle:()=>a,default:()=>o,frontMatter:()=>s,metadata:()=>d,toc:()=>c});var i=r(74848),l=r(28453);const s={},a="ISO GQL",d={id:"query/gql",title:"ISO GQL",description:"1.Introduction to ISO GQL",source:"@site/versions/version-4.5.1/en-US/source/8.query/2.gql.md",sourceDirName:"8.query",slug:"/query/gql",permalink:"/tugraph-db/en/4.5.1/query/gql",draft:!1,unlisted:!1,tags:[],version:"current",sidebarPosition:2,frontMatter:{},sidebar:"tutorialSidebar",previous:{title:"Cypher API",permalink:"/tugraph-db/en/4.5.1/query/cypher"},next:{title:"TuGraph Stored Procedure Guide",permalink:"/tugraph-db/en/4.5.1/olap&procedure/procedure/"}},t={},c=[{value:"1.Introduction to ISO GQL",id:"1introduction-to-iso-gql",level:2},{value:"2.List of Clauses",id:"2list-of-clauses",level:2},{value:"2.1.MATCH",id:"21match",level:3},{value:"Vertex basics",id:"vertex-basics",level:4},{value:"Get all vertices",id:"get-all-vertices",level:5},{value:"Get all vertices with a label",id:"get-all-vertices-with-a-label",level:5},{value:"Vertex matching with property",id:"vertex-matching-with-property",level:5},{value:"Vertex matching with filter",id:"vertex-matching-with-filter",level:5},{value:"Edge basics",id:"edge-basics",level:4},{value:"Edge pointing right",id:"edge-pointing-right",level:5},{value:"Edge pointing left",id:"edge-pointing-left",level:5},{value:"Edge matching with filter",id:"edge-matching-with-filter",level:5},{value:"Path matching",id:"path-matching",level:4},{value:"Variable length",id:"variable-length",level:5},{value:"2.2.OPTIONAL MATCH",id:"22optional-match",level:3},{value:"Match found",id:"match-found",level:4},{value:"Match Not Found",id:"match-not-found",level:4},{value:"2.3.RETURN",id:"23return",level:3},{value:"Return vertices",id:"return-vertices",level:4},{value:"Return edges",id:"return-edges",level:4},{value:"Return property",id:"return-property",level:4},{value:"Uncommon string used as variable name",id:"uncommon-string-used-as-variable-name",level:4},{value:"Alias",id:"alias",level:4},{value:"Optional property",id:"optional-property",level:4},{value:"Other expressions",id:"other-expressions",level:4},{value:"Distinct",id:"distinct",level:4},{value:"2.4.NEXT",id:"24next",level:3},{value:"Connecting MATCH clauses",id:"connecting-match-clauses",level:4},{value:"2.5.WHERE",id:"25where",level:3},{value:"Filter vertex",id:"filter-vertex",level:4},{value:"Filter edge",id:"filter-edge",level:4},{value:"Boolean expressions",id:"boolean-expressions",level:4},{value:"2.6.ORDER BY",id:"26order-by",level:3},{value:"Sorting the Result",id:"sorting-the-result",level:4},{value:"2.7.SKIP",id:"27skip",level:3},{value:"Without SKIP",id:"without-skip",level:4},{value:"Using SKIP",id:"using-skip",level:4},{value:"2.8.LIMIT",id:"28limit",level:3},{value:"Using LIMIT",id:"using-limit",level:4}];function h(e){const n={a:"a",code:"code",h1:"h1",h2:"h2",h3:"h3",h4:"h4",h5:"h5",header:"header",p:"p",pre:"pre",table:"table",tbody:"tbody",td:"td",th:"th",thead:"thead",tr:"tr",...(0,l.R)(),...e.components};return(0,i.jsxs)(i.Fragment,{children:[(0,i.jsx)(n.header,{children:(0,i.jsx)(n.h1,{id:"iso-gql",children:"ISO GQL"})}),"\n",(0,i.jsx)(n.h2,{id:"1introduction-to-iso-gql",children:"1.Introduction to ISO GQL"}),"\n",(0,i.jsxs)(n.p,{children:["Graph Query Language (GQL) is an upcoming International Standard language for property graph querying. It builds on the foundations of SQL and integrates proven ideas from the existing ",(0,i.jsx)(n.a,{href:"https://gql.today/comparing-cypher-pgql-and-g-core/",children:"openCypher, PGQL, GSQL, and G-CORE"})," languages. The standard is currently in the draft stage."]}),"\n",(0,i.jsxs)(n.p,{children:["TuGraph has implemented GQL based on the ",(0,i.jsx)(n.a,{href:"https://github.com/TuGraph-family/gql-grammar",children:"ISO GQL (ISO/IEC 39075) Antlr4 grammar file"}),". It includes some extensions and modifications. Not all GQL syntax is fully supported at the moment, but we will continue to improve and enhance it in the future."]}),"\n",(0,i.jsx)(n.h2,{id:"2list-of-clauses",children:"2.List of Clauses"}),"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n",(0,i.jsxs)(n.table,{children:[(0,i.jsx)(n.thead,{children:(0,i.jsxs)(n.tr,{children:[(0,i.jsx)(n.th,{children:"Category"}),(0,i.jsx)(n.th,{children:"Clauses"})]})}),(0,i.jsxs)(n.tbody,{children:[(0,i.jsxs)(n.tr,{children:[(0,i.jsx)(n.td,{children:"Reading clauses"}),(0,i.jsx)(n.td,{children:"MATCH"})]}),(0,i.jsxs)(n.tr,{children:[(0,i.jsx)(n.td,{}),(0,i.jsx)(n.td,{children:"OPTIONAL MATCH"})]}),(0,i.jsxs)(n.tr,{children:[(0,i.jsx)(n.td,{children:"Projecting clauses"}),(0,i.jsx)(n.td,{children:"RETURN"})]}),(0,i.jsxs)(n.tr,{children:[(0,i.jsx)(n.td,{}),(0,i.jsx)(n.td,{children:"NEXT"})]}),(0,i.jsxs)(n.tr,{children:[(0,i.jsx)(n.td,{children:"Reading sub-clauses"}),(0,i.jsx)(n.td,{children:"WHERE"})]}),(0,i.jsxs)(n.tr,{children:[(0,i.jsx)(n.td,{}),(0,i.jsx)(n.td,{children:"ORDER BY"})]}),(0,i.jsxs)(n.tr,{children:[(0,i.jsx)(n.td,{}),(0,i.jsx)(n.td,{children:"SKIP"})]}),(0,i.jsxs)(n.tr,{children:[(0,i.jsx)(n.td,{}),(0,i.jsx)(n.td,{children:"LIMIT"})]})]})]}),"\n",(0,i.jsx)(n.h3,{id:"21match",children:"2.1.MATCH"}),"\n",(0,i.jsxs)(n.p,{children:["The ",(0,i.jsx)(n.code,{children:"MATCH"})," clause is the most basic clause in GQL, and almost all queries are expanded through ",(0,i.jsx)(n.code,{children:"MATCH"}),"."]}),"\n",(0,i.jsxs)(n.p,{children:["The ",(0,i.jsx)(n.code,{children:"MATCH"})," clause is used to specify the matching pattern to search in the graph, to match vertices or paths that meet certain conditions."]}),"\n",(0,i.jsx)(n.h4,{id:"vertex-basics",children:"Vertex basics"}),"\n",(0,i.jsx)(n.h5,{id:"get-all-vertices",children:"Get all vertices"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:"MATCH (n)\nRETURN n\n"})}),"\n",(0,i.jsx)(n.h5,{id:"get-all-vertices-with-a-label",children:"Get all vertices with a label"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:"MATCH (n:Person)\nRETURN n\n"})}),"\n",(0,i.jsx)(n.h5,{id:"vertex-matching-with-property",children:"Vertex matching with property"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:"MATCH (n:Person{name:'Michael Redgrave'})\nRETURN n.birthyear\n"})}),"\n",(0,i.jsx)(n.p,{children:"return"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{className:"language-JSON",children:'[{"n.birthyear":1908}]\n'})}),"\n",(0,i.jsx)(n.h5,{id:"vertex-matching-with-filter",children:"Vertex matching with filter"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:"MATCH (n:Person WHERE n.birthyear > 1910)\nRETURN n.name LIMIT 2\n"})}),"\n",(0,i.jsx)(n.p,{children:"return"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{className:"language-JSON",children:'[{"n.name":"Christopher Nolan"},{"n.name":"Corin Redgrave"}]\n'})}),"\n",(0,i.jsx)(n.h4,{id:"edge-basics",children:"Edge basics"}),"\n",(0,i.jsx)(n.h5,{id:"edge-pointing-right",children:"Edge pointing right"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:"MATCH (n:Person WHERE n.birthyear = 1970)-[e]->(m)\nRETURN n.name, label(e), m.name\n"})}),"\n",(0,i.jsx)(n.p,{children:"return"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{className:"language-JSON",children:'[{"label(e)":"BORN_IN","m.name":"London","n.name":"Christopher Nolan"},{"label(e)":"DIRECTED","m.name":null,"n.name":"Christopher Nolan"}]\n'})}),"\n",(0,i.jsx)(n.h5,{id:"edge-pointing-left",children:"Edge pointing left"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:"MATCH (n:Person WHERE n.birthyear = 1939)<-[e]-(m)\nRETURN n.name, label(e), m.name\n"})}),"\n",(0,i.jsx)(n.p,{children:"return"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{className:"language-JSON",children:'[{"label(e)":"HAS_CHILD","m.name":"Rachel Kempson","n.name":"Corin Redgrave"},{"label(e)":"HAS_CHILD","m.name":"Michael Redgrave","n.name":"Corin Redgrave"}]\n'})}),"\n",(0,i.jsx)(n.h5,{id:"edge-matching-with-filter",children:"Edge matching with filter"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:"MATCH (n:Person)-[e:BORN_IN WHERE e.weight > 20]->(m)\nRETURN n.name, e.weight, m.name\n"})}),"\n",(0,i.jsx)(n.p,{children:"return"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{className:"language-JSON",children:'[{"e.weight":20.549999237060547,"m.name":"New York","n.name":"John Williams"},{"e.weight":20.6200008392334,"m.name":"New York","n.name":"Lindsay Lohan"}]\n'})}),"\n",(0,i.jsx)(n.h4,{id:"path-matching",children:"Path matching"}),"\n",(0,i.jsx)(n.h5,{id:"variable-length",children:"Variable length"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:"MATCH (n:Person)-[e]->{2,3}(m:Person)\nRETURN m.name LIMIT 2\n"})}),"\n",(0,i.jsx)(n.p,{children:"return"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{className:"language-JSON",children:'[{"m.name":"Liam Neeson"},{"m.name":"Natasha Richardson"}]\n'})}),"\n",(0,i.jsx)(n.h3,{id:"22optional-match",children:"2.2.OPTIONAL MATCH"}),"\n",(0,i.jsxs)(n.p,{children:["The ",(0,i.jsx)(n.code,{children:"OPTIONAL MATCH"})," clause matches a graph pattern and returns ",(0,i.jsx)(n.code,{children:"null"})," if there is no match."]}),"\n",(0,i.jsx)(n.h4,{id:"match-found",children:"Match found"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:"OPTIONAL MATCH (n:Person{name:'Michael Redgrave'})\nRETURN n.birthyear\n"})}),"\n",(0,i.jsx)(n.p,{children:"return"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{className:"language-JSON",children:'[{"n.birthyear":1908}]\n'})}),"\n",(0,i.jsx)(n.h4,{id:"match-not-found",children:"Match Not Found"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:"OPTIONAL MATCH (n:Person{name:'Redgrave Michael'})\nRETURN n.birthyear\n"})}),"\n",(0,i.jsx)(n.p,{children:"return"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{className:"language-JSON",children:'[{"n.birthyear":null}]\n'})}),"\n",(0,i.jsx)(n.h3,{id:"23return",children:"2.3.RETURN"}),"\n",(0,i.jsxs)(n.p,{children:["The ",(0,i.jsx)(n.code,{children:"RETURN"})," clause specifies the results to be returned, including vertices, edges, paths, properties, etc."]}),"\n",(0,i.jsx)(n.h4,{id:"return-vertices",children:"Return vertices"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:"MATCH (n)\nRETURN n LIMIT 2\n"})}),"\n",(0,i.jsx)(n.p,{children:"return"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{className:"language-JSON",children:'[{"n":{"identity":0,"label":"Person","properties":{"birthyear":1910,"name":"Rachel Kempson"}}},{"n":{"identity":1,"label":"Person","properties":{"birthyear":1908,"name":"Michael Redgrave"}}}]\n'})}),"\n",(0,i.jsx)(n.h4,{id:"return-edges",children:"Return edges"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:"MATCH (n)-[e]->(m)\nRETURN e LIMIT 2\n"})}),"\n",(0,i.jsx)(n.p,{children:"return"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{className:"language-JSON",children:'[{"e":{"dst":2,"forward":false,"identity":0,"label":"HAS_CHILD","label_id":0,"src":0,"temporal_id":0}},{"e":{"dst":3,"forward":false,"identity":0,"label":"HAS_CHILD","label_id":0,"src":0,"temporal_id":0}}]\n'})}),"\n",(0,i.jsx)(n.h4,{id:"return-property",children:"Return property"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:"MATCH (n:Person)\nRETURN n.name LIMIT 2\n"})}),"\n",(0,i.jsx)(n.p,{children:"return"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{className:"language-JSON",children:'[{"n.name":"Christopher Nolan"},{"n.name":"Corin Redgrave"}]\n'})}),"\n",(0,i.jsx)(n.h4,{id:"uncommon-string-used-as-variable-name",children:"Uncommon string used as variable name"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:"MATCH (`/uncommon variable`:Person)\nRETURN `/uncommon variable`.name LIMIT 3\n"})}),"\n",(0,i.jsx)(n.p,{children:"return"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{className:"language-JSON",children:'[{"`/uncommon variable`.name":"Christopher Nolan"},{"`/uncommon variable`.name":"Corin Redgrave"},{"`/uncommon variable`.name":"Dennis Quaid"}]\n'})}),"\n",(0,i.jsx)(n.h4,{id:"alias",children:"Alias"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:"MATCH (n:Person)\nRETURN n.name AS nname LIMIT 2\n"})}),"\n",(0,i.jsx)(n.p,{children:"return"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{className:"language-JSON",children:'[{"nname":"Christopher Nolan"},{"nname":"Corin Redgrave"}]\n'})}),"\n",(0,i.jsx)(n.h4,{id:"optional-property",children:"Optional property"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:"MATCH (n:Person)\nRETURN n.age LIMIT 2\n"})}),"\n",(0,i.jsx)(n.p,{children:"return"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{className:"language-JSON",children:'[{"n.age":null},{"n.age":null}]\n'})}),"\n",(0,i.jsx)(n.h4,{id:"other-expressions",children:"Other expressions"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:'MATCH (n:Person)\nRETURN n.birthyear > 1970, "I\'m a literal", 1 + 2, abs(-2)\nLIMIT 2\n'})}),"\n",(0,i.jsx)(n.p,{children:"return"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{className:"language-JSON",children:'[{"\\"I\'m a literal\\"":"I\'m a literal","1 + 2":3,"abs(-2)":2,"n.birthyear > 1970":false},{"\\"I\'m a literal\\"":"I\'m a literal","1 + 2":3,"abs(-2)":2,"n.birthyear > 1970":false}]\n'})}),"\n",(0,i.jsx)(n.h4,{id:"distinct",children:"Distinct"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:"MATCH (n)\nRETURN DISTINCT label(n) AS label\n"})}),"\n",(0,i.jsx)(n.p,{children:"return"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{className:"language-JSON",children:'[{"label":"Person"},{"label":"City"},{"label":"Film"}]\n'})}),"\n",(0,i.jsx)(n.h3,{id:"24next",children:"2.4.NEXT"}),"\n",(0,i.jsxs)(n.p,{children:["The ",(0,i.jsx)(n.code,{children:"NEXT"})," clause is used to connect multiple clauses."]}),"\n",(0,i.jsx)(n.h4,{id:"connecting-match-clauses",children:"Connecting MATCH clauses"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:"MATCH (n:Person) WHERE n.birthyear = 1970\nRETURN n\nNEXT\nMATCH (m:Person) WHERE m.birthyear < 1968\nRETURN n.name, n.birthyear, m.name LIMIT 2\n"})}),"\n",(0,i.jsx)(n.p,{children:"return"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{className:"language-JSON",children:'[{"m.name":"Rachel Kempson","n.birthyear":1970,"n.name":"Christopher Nolan"},{"m.name":"Michael Redgrave","n.birthyear":1970,"n.name":"Christopher Nolan"}]\n'})}),"\n",(0,i.jsx)(n.h3,{id:"25where",children:"2.5.WHERE"}),"\n",(0,i.jsxs)(n.p,{children:[(0,i.jsx)(n.code,{children:"WHERE"})," clause is used to filter records."]}),"\n",(0,i.jsx)(n.h4,{id:"filter-vertex",children:"Filter vertex"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:"MATCH (n:Person WHERE n.birthyear > 1965)\nRETURN n.name\n"})}),"\n",(0,i.jsx)(n.p,{children:"returns"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{className:"language-JSON",children:'[{"n.name":"Christopher Nolan"},{"n.name":"Lindsay Lohan"}]\n'})}),"\n",(0,i.jsx)(n.h4,{id:"filter-edge",children:"Filter edge"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:"MATCH (n:Person WHERE n.birthyear > 1965)-[e:ACTED_IN]->(m:Film)\nWHERE e.charactername = 'Halle/Annie'\nRETURN m.title\n"})}),"\n",(0,i.jsx)(n.p,{children:"returns"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{className:"language-JSON",children:'[{"m.title":"The Parent Trap"}]\n'})}),"\n",(0,i.jsx)(n.h4,{id:"boolean-expressions",children:"Boolean expressions"}),"\n",(0,i.jsxs)(n.p,{children:[(0,i.jsx)(n.code,{children:"AND"}),", ",(0,i.jsx)(n.code,{children:"OR"}),", ",(0,i.jsx)(n.code,{children:"XOR"}),", and ",(0,i.jsx)(n.code,{children:"NOT"})," Boolean expressions can be used in the ",(0,i.jsx)(n.code,{children:"WHERE"})," clause to filter data."]}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:"MATCH (n:Person)\nWHERE\n\tn.birthyear > 1930 AND (n.birthyear < 1950 OR n.name = 'Corin Redgrave')\nRETURN n LIMIT 2\n"})}),"\n",(0,i.jsx)(n.p,{children:"returns"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{className:"language-JSON",children:'[{"n":{"identity":3,"label":"Person","properties":{"birthyear":1939,"name":"Corin Redgrave"}}},{"n":{"identity":11,"label":"Person","properties":{"birthyear":1932,"name":"John Williams"}}}]\n'})}),"\n",(0,i.jsx)(n.h3,{id:"26order-by",children:"2.6.ORDER BY"}),"\n",(0,i.jsxs)(n.p,{children:[(0,i.jsx)(n.code,{children:"ORDER BY"})," is a clause of ",(0,i.jsx)(n.code,{children:"RETURN"})," that sorts the output result."]}),"\n",(0,i.jsx)(n.h4,{id:"sorting-the-result",children:"Sorting the Result"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:"MATCH (n:Person WHERE n.birthyear < 1970)\nRETURN n.birthyear AS q\nORDER BY q ASC\nLIMIT 5\n"})}),"\n",(0,i.jsx)(n.p,{children:"return"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{className:"language-JSON",children:'[{"q":1873},{"q":1908},{"q":1910},{"q":1930},{"q":1932}]\n'})}),"\n",(0,i.jsx)(n.h3,{id:"27skip",children:"2.7.SKIP"}),"\n",(0,i.jsxs)(n.p,{children:[(0,i.jsx)(n.code,{children:"SKIP"})," specifies the offset of the result rows."]}),"\n",(0,i.jsx)(n.h4,{id:"without-skip",children:"Without SKIP"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:"MATCH (n:Person)\nRETURN n.name LIMIT 3\n"})}),"\n",(0,i.jsx)(n.p,{children:"return"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{className:"language-JSON",children:'[{"n.name":"Christopher Nolan"},{"n.name":"Corin Redgrave"},{"n.name":"Dennis Quaid"}]\n'})}),"\n",(0,i.jsx)(n.h4,{id:"using-skip",children:"Using SKIP"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:"MATCH (n:Person)\nRETURN n.name SKIP 1 LIMIT 2\n"})}),"\n",(0,i.jsx)(n.p,{children:"return"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{className:"language-JSON",children:'[{"n.name":"Corin Redgrave"},{"n.name":"Dennis Quaid"}]\n'})}),"\n",(0,i.jsx)(n.h3,{id:"28limit",children:"2.8.LIMIT"}),"\n",(0,i.jsxs)(n.p,{children:["The ",(0,i.jsx)(n.code,{children:"LIMIT"})," clause is used to limit the number of rows in the result."]}),"\n",(0,i.jsx)(n.h4,{id:"using-limit",children:"Using LIMIT"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{children:"MATCH (n:Person)\nRETURN n.name LIMIT 2;\n"})}),"\n",(0,i.jsx)(n.p,{children:"return"}),"\n",(0,i.jsx)(n.pre,{children:(0,i.jsx)(n.code,{className:"language-JSON",children:'[{"n.name":"Christopher Nolan"},{"n.name":"Corin Redgrave"}]\n'})})]})}function o(e={}){const{wrapper:n}={...(0,l.R)(),...e.components};return n?(0,i.jsx)(n,{...e,children:(0,i.jsx)(h,{...e})}):h(e)}},28453:(e,n,r)=>{r.d(n,{R:()=>a,x:()=>d});var i=r(96540);const l={},s=i.createContext(l);function a(e){const n=i.useContext(s);return i.useMemo((function(){return"function"==typeof e?e(n):{...n,...e}}),[n,e])}function d(e){let n;return n=e.disableParentContext?"function"==typeof e.components?e.components(l):e.components||l:a(e.components),i.createElement(s.Provider,{value:n},e.children)}}}]);
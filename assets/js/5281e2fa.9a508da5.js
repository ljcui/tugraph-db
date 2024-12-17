"use strict";(self.webpackChunkdocusaurus=self.webpackChunkdocusaurus||[]).push([[49565],{15431:(e,n,a)=>{a.r(n),a.d(n,{assets:()=>l,contentTitle:()=>s,default:()=>g,frontMatter:()=>i,metadata:()=>o,toc:()=>d});var t=a(74848),r=a(28453);const i={},s="Training",o={id:"developer-manual/interface/learn/training",title:"Training",description:"This document details how to use TuGraph for training Graph Neural Networks (GNNs).",source:"@site/versions/version-4.0.1/en-US/source/5.developer-manual/6.interface/5.learn/3.training.md",sourceDirName:"5.developer-manual/6.interface/5.learn",slug:"/developer-manual/interface/learn/training",permalink:"/tugraph-db/en/4.0.1/developer-manual/interface/learn/training",draft:!1,unlisted:!1,tags:[],version:"current",sidebarPosition:3,frontMatter:{},sidebar:"tutorialSidebar",previous:{title:"Sampling API",permalink:"/tugraph-db/en/4.0.1/developer-manual/interface/learn/sampling_api"},next:{title:"Unit Testing",permalink:"/tugraph-db/en/4.0.1/developer-manual/quality/unit-testing"}},l={},d=[{value:"1. Training",id:"1-training",level:2},{value:"2. Mini-Batch Training",id:"2-mini-batch-training",level:2},{value:"3. Full-Batch training",id:"3-full-batch-training",level:2}];function h(e){const n={blockquote:"blockquote",code:"code",h1:"h1",h2:"h2",header:"header",p:"p",pre:"pre",table:"table",tbody:"tbody",td:"td",th:"th",thead:"thead",tr:"tr",...(0,r.R)(),...e.components};return(0,t.jsxs)(t.Fragment,{children:[(0,t.jsx)(n.header,{children:(0,t.jsx)(n.h1,{id:"training",children:"Training"})}),"\n",(0,t.jsxs)(n.blockquote,{children:["\n",(0,t.jsx)(n.p,{children:"This document details how to use TuGraph for training Graph Neural Networks (GNNs)."}),"\n"]}),"\n",(0,t.jsx)(n.h2,{id:"1-training",children:"1. Training"}),"\n",(0,t.jsx)(n.p,{children:"When using TuGraph's graph learning module for training, it can be divided into two categories: full graph training and mini-batch training."}),"\n",(0,t.jsx)(n.p,{children:"Full graph training involves loading the entire graph from the TuGraph db into memory and training the GNN. Mini-batch training uses the sampling operator of the TuGraph graph learning module to sample the entire graph data and then input it into the training framework for training."}),"\n",(0,t.jsx)(n.h2,{id:"2-mini-batch-training",children:"2. Mini-Batch Training"}),"\n",(0,t.jsx)(n.p,{children:"Mini-batch training requires the use of TuGraph's graph learning module's sampling operators, which currently support Neighbor Sampling, Edge Sampling, Random Walk Sampling, and Negative Sampling. The sampling result of the TuGraph graph learning module's sampling operator is returned in the form of a List."}),"\n",(0,t.jsx)(n.p,{children:"The following takes Neighbor Sampling as an example to introduce how to convert the sampled results into the training framework for format conversion.\nThe user needs to provide a Sample class:"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-python",children:"class TuGraphSample(object):\n    def __init__(self, args=None):\n        super(TuGraphSample, self).__init__()\n        self.args = args\n    def sample(self, g, seed_nodes):\n        args = self.args\n        # 1. Load graph data\n        galaxy = PyGalaxy(args.db_path)\n        galaxy.SetCurrentUser(args.username, args.password)\n        db = galaxy.OpenGraph(args.graph_name, False)\n        sample_node = seed_nodes.tolist()\n        length = args.randomwalk_length\n        NodeInfo = []\n        EdgeInfo = []\n        # 2. Sampling method, the result is stored in NodeInfo and EdgeInfo\n        if args.sample_method == 'randomwalk':\n            randomwalk.Process(db, 100, sample_node, length, NodeInfo, EdgeInfo)\n        elif args.sample_method == 'negative':\n            negativesample.Process(db, 100)\n        else:\n            neighborsample(db, 100, sample_node, args.nbor_sample_num, NodeInfo, EdgeInfo)\n        del db\n        del galaxy\n        # 3. Format conversion of the result to fit the training format\n        src = EdgeInfo[0].astype('int64')\n        dst = EdgeInfo[1].astype('int64')\n        nodes_idx = NodeInfo[0].astype('int64')\n        remap(src, dst, nodes_idx)\n        features = NodeInfo[1].astype('float32')\n        labels = NodeInfo[2].astype('int64')\n        g = dgl.graph((src, dst))\n        g.ndata['feat'] = torch.tensor(features)\n        g.ndata['label'] = torch.tensor(labels)\n        return g\n"})}),"\n",(0,t.jsx)(n.p,{children:"As shown in the code, the graph data is first loaded into memory. Then use the sampling operator to sample the graph data, and the results are stored in NodeInfo and EdgeInfo. NodeInfo and EdgeInfo are python list results, and the stored information results are as follows:"}),"\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n",(0,t.jsxs)(n.table,{children:[(0,t.jsx)(n.thead,{children:(0,t.jsxs)(n.tr,{children:[(0,t.jsx)(n.th,{children:"Graph Data"}),(0,t.jsx)(n.th,{children:"Storage Position"})]})}),(0,t.jsxs)(n.tbody,{children:[(0,t.jsxs)(n.tr,{children:[(0,t.jsx)(n.td,{children:"Edge source"}),(0,t.jsx)(n.td,{children:"EdgeInfo[0]"})]}),(0,t.jsxs)(n.tr,{children:[(0,t.jsx)(n.td,{children:"Edge destination"}),(0,t.jsx)(n.td,{children:"EdgeInfo[1]"})]}),(0,t.jsxs)(n.tr,{children:[(0,t.jsx)(n.td,{children:"Vertex ID"}),(0,t.jsx)(n.td,{children:"NodeInfo[0]"})]}),(0,t.jsxs)(n.tr,{children:[(0,t.jsx)(n.td,{children:"Vertex features"}),(0,t.jsx)(n.td,{children:"NodeInfo[1]"})]}),(0,t.jsxs)(n.tr,{children:[(0,t.jsx)(n.td,{children:"Vertex label"}),(0,t.jsx)(n.td,{children:"NodeInfo[2]"})]})]})]}),"\n",(0,t.jsx)(n.p,{children:"Finally, we format the result to fit the training format. Here we use the DGL training framework, so we construct a DGL Graph using the result data, and then return the DGL Graph."}),"\n",(0,t.jsx)(n.p,{children:"Once we provide the TuGraphSample class, we can use it for Mini-Batch training. Let the data loading part of DGL use the sampler instance of TuGraphSample as follows:"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-python",children:"    sampler = TugraphSample(args)\n    fake_g = construct_graph() # just make dgl happy\n    dataloader = dgl.dataloading.DataLoader(fake_g,\n        torch.arange(train_nids),\n        sampler,\n        batch_size=batch_size,\n        device=device,\n        use_ddp=True,\n        num_workers=0,\n        drop_last=False,\n        )\n"})}),"\n",(0,t.jsx)(n.p,{children:"Use DGL for model training:"}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-python",children:"def train(dataloader, model):\n    optimizer = torch.optim.Adam(model.parameters(), lr=1e-2, weight_decay=5e-4)\n    model.train()\n    s = time.time()\n    for graph in dataloader:\n        load_time = time.time()\n        graph = dgl.add_self_loop(graph)\n        logits = model(graph, graph.ndata['feat'])\n        loss = loss_fcn(logits, graph.ndata['label'])\n        optimizer.zero_grad()\n        loss.backward()\n        optimizer.step()\n        train_time = time.time()\n        print('load time', load_time - s, 'train_time', train_time - load_time)\n        s = time.time()\n    return float(loss)\n"})}),"\n",(0,t.jsx)(n.h2,{id:"3-full-batch-training",children:"3. Full-Batch training"}),"\n",(0,t.jsx)(n.p,{children:"Full-Batch training of GNNs (Graph Neural Networks) is a type of training that involves processing the entire training dataset at once. It is one of the simplest and most straightforward training methods for GNNs, where the entire graph is treated as a single instance. In full-batch training, the entire dataset is loaded into memory and the model is trained on the entire graph. This type of training is especially useful for small to medium-sized graphs, and is mainly used for static graphs that do not change over time."}),"\n",(0,t.jsx)(n.pre,{children:(0,t.jsx)(n.code,{className:"language-python",children:"getdb. Process(db, olapondb, feature_len, NodeInfo, EdgeInfo)\n"})}),"\n",(0,t.jsx)(n.p,{children:"The full graph are then fed into the training framework for training.\nFull code: learn/examples/train_full_cora.py\u3002"})]})}function g(e={}){const{wrapper:n}={...(0,r.R)(),...e.components};return n?(0,t.jsx)(n,{...e,children:(0,t.jsx)(h,{...e})}):h(e)}},28453:(e,n,a)=>{a.d(n,{R:()=>s,x:()=>o});var t=a(96540);const r={},i=t.createContext(r);function s(e){const n=t.useContext(i);return t.useMemo((function(){return"function"==typeof e?e(n):{...n,...e}}),[n,e])}function o(e){let n;return n=e.disableParentContext?"function"==typeof e.components?e.components(r):e.components||r:s(e.components),t.createElement(i.Provider,{value:n},e.children)}}}]);
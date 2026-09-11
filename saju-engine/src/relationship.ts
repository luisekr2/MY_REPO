import type { Element } from './types.js';
import { runFullSajuEngine } from './full-engine.js';
import type { ManseryeokAdapterInput } from './traditional.js';

export interface DirectionalSupply {
  from: 'A'|'B';
  to: 'A'|'B';
  element: Element;
  recipientNeed: number;
  recipientNatalAvailability: number;
  providerAvailability: number;
  suppliedGap: number;
  overloadRisk: number;
  evidence: string[];
}

export interface RelationshipDimension {
  score: number;
  confidence: number;
  evidence: string[];
}

export interface DyadicInteractionGraph {
  version: string;
  directionalSupply: {
    aToB: DirectionalSupply[];
    bToA: DirectionalSupply[];
  };
  dimensions: {
    attraction: RelationshipDimension;
    communication: RelationshipDimension;
    conflict: RelationshipDimension;
    powerBalance: RelationshipDimension;
    autonomy: RelationshipDimension;
    stability: RelationshipDimension;
    growthComplementarity: RelationshipDimension;
    practicalFit: RelationshipDimension;
    romanticChemistry: RelationshipDimension;
  };
  asymmetry: {
    aReceives: number;
    bReceives: number;
    imbalance: number;
    evidence: string[];
  };
  relationshipConfidence: number;
  timingSynchrony: {
    status: 'not_evaluated';
    note: string;
  };
}

function clamp(n:number,a=0,b=100){ return Math.max(a,Math.min(b,n)); }
function avg(xs:number[]){ return xs.length ? xs.reduce((a,b)=>a+b,0)/xs.length : 0; }

function directional(
  from:'A'|'B',
  to:'A'|'B',
  provider:ReturnType<typeof runFullSajuEngine>,
  recipient:ReturnType<typeof runFullSajuEngine>
):DirectionalSupply[] {
  const out:DirectionalSupply[]=[];
  for(const req of recipient.advanced.availabilityGraph.required){
    const providerNode=provider.advanced.availabilityGraph.natal[req.element];
    const recipientNode=recipient.advanced.availabilityGraph.natal[req.element];
    const gap=Math.max(0, req.need-recipientNode.accessible);
    const suppliedGap=Math.min(gap, providerNode.accessible);
    const overloadRisk=Math.max(
      0,
      providerNode.accessible + recipientNode.accessible - Math.max(req.need,.55)
    );

    out.push({
      from,to,element:req.element,
      recipientNeed:req.need,
      recipientNatalAvailability:recipientNode.accessible,
      providerAvailability:providerNode.accessible,
      suppliedGap,
      overloadRisk:Math.min(1,overloadRisk),
      evidence:[
        `recipient need=${req.need.toFixed(2)}`,
        `recipient natal availability=${recipientNode.accessible.toFixed(2)}`,
        `provider availability=${providerNode.accessible.toFixed(2)}`,
        `supplied gap=${suppliedGap.toFixed(2)}`
      ]
    });
  }
  return out.sort((a,b)=>b.suppliedGap-a.suppliedGap);
}

function scoreDimension(score:number, confidence:number, evidence:string[]):RelationshipDimension{
  return {score:Math.round(clamp(score)*10)/10,confidence:Math.max(0,Math.min(1,confidence)),evidence};
}

export function runRelationshipEngine(
  aInput:ManseryeokAdapterInput,
  bInput:ManseryeokAdapterInput
):DyadicInteractionGraph {
  const A=runFullSajuEngine(aInput);
  const B=runFullSajuEngine(bInput);

  const aToB=directional('A','B',A,B);
  const bToA=directional('B','A',B,A);

  const aSupply=aToB.reduce((s,x)=>s+x.suppliedGap,0);
  const bSupply=bToA.reduce((s,x)=>s+x.suppliedGap,0);
  const totalSupply=aSupply+bSupply;
  const imbalance=totalSupply ? Math.abs(aSupply-bSupply)/totalSupply : 0;

  const aOver=avg(aToB.map(x=>x.overloadRisk));
  const bOver=avg(bToA.map(x=>x.overloadRisk));
  const overload=avg([aOver,bOver]);

  const aFeat=A.interpretation.features;
  const bFeat=B.interpretation.features;
  const f=(obj:typeof aFeat,key:string)=>obj[key]?.score ?? 50;

  const expressionGap=Math.abs(f(aFeat,'emotionalExpression')-f(bFeat,'emotionalExpression'))/100;
  const independenceGap=Math.abs(
    f(aFeat,'independenceInLove')-f(bFeat,'independenceInLove')
  )/100;
  const structureGap=Math.abs(f(aFeat,'structurePreference')-f(bFeat,'structurePreference'))/100;
  const moneyGap=Math.abs(f(aFeat,'resourceManagement')-f(bFeat,'resourceManagement'))/100;

  const relA=A.traditional.relations;
  const relB=B.traditional.relations;
  const internalClash=(rels:typeof relA)=>rels
    .filter(r=>['branch_clash','punishment','harm','break'].includes(r.type))
    .reduce((s,r)=>s+r.strength,0);
  const baselineStress=Math.min(1,(internalClash(relA)+internalClash(relB))/6);

  const complement=Math.min(1,totalSupply/1.6);
  const mutuality=Math.min(1,Math.min(aSupply,bSupply)/.55);

  const attraction=50 + complement*24 + mutuality*12 - overload*10;
  const communication=65 - expressionGap*28 - baselineStress*10 + mutuality*10;
  const conflict=25 + baselineStress*28 + expressionGap*18 + overload*18 + structureGap*12;
  const powerBalance=82 - imbalance*50 - independenceGap*18;
  const autonomy=62 - independenceGap*30 - overload*12;
  const stability=58 + mutuality*16 - imbalance*22 - baselineStress*16 - overload*14;
  const growth=48 + complement*38 - overload*12;
  const practical=65 - moneyGap*26 - structureGap*20 + mutuality*8;
  const chemistry=52 + complement*22 + (1-expressionGap)*8 - overload*10;

  const evidBase=[
    `A receives=${bSupply.toFixed(2)}`,
    `B receives=${aSupply.toFixed(2)}`,
    `imbalance=${imbalance.toFixed(2)}`,
    `complement=${complement.toFixed(2)}`,
    `overload=${overload.toFixed(2)}`
  ];
  const confidence=Math.min(
    .92,
    avg([
      A.interpretation.features.independence?.confidence ?? .5,
      B.interpretation.features.independence?.confidence ?? .5,
      A.advanced.structureResolution.useful.confidence,
      B.advanced.structureResolution.useful.confidence
    ])
  );

  return {
    version:'0.1.0',
    directionalSupply:{aToB,bToA},
    dimensions:{
      attraction:scoreDimension(attraction,confidence,[...evidBase,'functional complement + mutuality']),
      communication:scoreDimension(communication,confidence,[...evidBase,`expressionGap=${expressionGap.toFixed(2)}`]),
      conflict:scoreDimension(conflict,confidence,[...evidBase,`baselineStress=${baselineStress.toFixed(2)}`]),
      powerBalance:scoreDimension(powerBalance,confidence,[...evidBase,`independenceGap=${independenceGap.toFixed(2)}`]),
      autonomy:scoreDimension(autonomy,confidence,[...evidBase,`independenceGap=${independenceGap.toFixed(2)}`]),
      stability:scoreDimension(stability,confidence,[...evidBase,'mutual supply vs stress/imbalance']),
      growthComplementarity:scoreDimension(growth,confidence,[...evidBase,'unmet functions supplied by partner']),
      practicalFit:scoreDimension(practical,confidence,[...evidBase,`moneyGap=${moneyGap.toFixed(2)}`,`structureGap=${structureGap.toFixed(2)}`]),
      romanticChemistry:scoreDimension(chemistry,confidence,[...evidBase,`expressionGap=${expressionGap.toFixed(2)}`])
    },
    asymmetry:{
      aReceives:bSupply,
      bReceives:aSupply,
      imbalance,
      evidence:[
        `B→A supplied gap=${bSupply.toFixed(2)}`,
        `A→B supplied gap=${aSupply.toFixed(2)}`
      ]
    },
    relationshipConfidence:confidence,
    timingSynchrony:{
      status:'not_evaluated',
      note:'v0.1 reserves this slot for future A/B daeun·annual synchrony analysis.'
    }
  };
}

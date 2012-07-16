// this is the tree grammar that postprocesses DataPath grammar
tree grammar DPtree;

options {
  language = C;
  tokenVocab = BaseLexer;
  ASTLabelType    = pANTLR3_BASE_TREE;
}

@header {
 #include "SymbolicWaypointConfig.h"
 #include "LemonTranslator.h"
 #include "AttributeManager.h"
 #include "QueryManager.h"
 #include "DataTypeManager.h"
 #include "ExprListInfo.h"
 #include "Catalog.h"
 #include <iostream>
 #include <map>
 #include <vector>

/* Debugging */
#undef PREPORTERROR
#define PREPORTERROR assert(1=2)

// uncomment this to enforce types
#define ENFORCE_TYPES
//#define ENFORCE_GLA_TYPES

#ifndef TXT
#define TXT(x) ((const char*)(x->getText(x))->chars)
#endif
#ifndef TXTN
#define TXTN(x) ((NormalizeQuotes((const char*)(x->getText(x))->chars)).c_str())
#endif
#ifndef TXTS
#define TXTS(x) ((StripQuotes((const char*)(x->getText(x))->chars)).c_str())
#endif
#ifndef STR
#define STR(X) ( string(TXT(X)) )
#endif
#ifndef STRN
#define STRN(X) ( string(TXTN(X)) )
#endif
#ifndef STRS
#define STRS(X) ( string(TXTS(X)) )
#endif
#ifndef ADD_CST
#define ADD_CST(cstStr, cst) ((cstStr) += "    " + (cst))
#endif
}

@members {
static LemonTranslator* lT = NULL; // this is a pointe to the lemon translator used
 //set by parse to be used by all
static WayPointID wp; // this is the currect waypoint. If null, illegal to have a waypoint
 // related statement
static WaypointType wpType = InvalidWaypoint; //type of above waypoint
static QueryID qry; // current query. Empty means illegal ...

static bool isNew = false; // is this a new anything.

 // attribute manager that does the translation from attributes to SlotID
 // when it starts it will define the attributes of all relations
static AttributeManager& am = AttributeManager::GetAttributeManager();

 // get it ready for adding queries
static QueryManager& qm = QueryManager::GetQueryManager();

 // get the datatype manager
static DataTypeManager& dTM = DataTypeManager::GetDataTypeManager();

// the catalog
static Catalog& catalog = Catalog::GetCatalog();

extern int tempCounter; // id for temporary variables}
string StripQuotes(string str);
string NormalizeQuotes(string str);
string GenerateTemp(const char* pattern);

}

parse[LemonTranslator* trans] : {
      lT = trans;
    } complexStatement+ ;

complexStatement
  : ^(NEWSTATEMENT statement[true])
  | ^(OLDSTATEMENT statement[false])
  | ^(DELWAYPOINT ID) /* nothing for now, add */
  | ^(DELQUERY ID) { QueryID q=qm.GetQueryID(TXT($ID)); lT->DeleteQuery(q); }
  | ^(CRDATATYPE ID s=STRING) { dTM.AddBaseType(STR($ID), STRS($s)); }
  | ^(CRSYNONIM tp=ID s=ID) { dTM.AddSynonymType(STR($tp), STR($s)); }
  | ^(FUNCTION ID (s=STRING)? dType lstArgsFc){ dTM.AddFunctions(STR($ID), $lstArgsFc.vecT, $dType.type, true); /* string ignored for now */ }
  | ^(OPDEF n=STRING (s=STRING)? dType lstArgsFc){ dTM.AddFunctions(STRS($n), $lstArgsFc.vecT, $dType.type, true); /* string ignored for now */ }
  | ^(CRGLA ID (s=STRING)? ^(TPATT (ret=lstArgsGLA)) ^(TPATT (args=lstArgsGLA))) { dTM.AddGLA(STR($ID), $args.vecT, $ret.vecT); }
  | relationCR
  | FLUSHTOKEN {dTM.Save(); catalog.SaveCatalog();}
  | runStmt
  | QUITTOKEN { exit(0); }
  ;

runStmt
@init{QueryIDSet qrys;}
@after{lT->Run(qrys);}
  : ^(RUN__ (ID{
        QueryID lq = qm.GetQueryID(TXT($ID));
        // check lq to ensur it is valid
        qrys.Union(lq);
        })+)
  ;

relationCR
@init { Schema newSch; int index = 0; }
@after { catalog.AddSchema(newSch);/* register the relatin with catalog */ }
  : ^(CRRELATION x=ID {newSch.SetRelationName(TXT($x));/* set relation name */}
      ( ^(TPATT n=ID t=ID)
        { Attribute att; att.SetName(TXT($n)); string ty(string(TXT($t))); att.SetType(ty); att.SetIndex(++index); newSch.AddAttribute(att);/* add attribte n with type t */ }
      )+ )
  ;

lstArgsFc returns [vector<string> vecT]
  :( dType { vecT.push_back($dType.type); } )*
  ;

lstArgsGLA returns [vector<string> vecT]
    : f=dType {vecT.push_back($f.type);} ( o=dType {vecT.push_back($o.type); } )*
    ;

dType returns[string type]
  : ID {$type=TXT($ID);}
  ;

statement[bool isNew]
    :    scanner
    | waypoint[isNew]
    | query
    | writer
    ;

writer :
    ^(WRITER__ a=ID b=ID {
        // set wp to current scanner
        wp = WayPointID::GetIdByName((const char*)$a.text->chars);
        // set the query
        SlotContainer attribs;
        am.GetAttributesSlots(TXT($a), attribs); // put attributes in attribs
        WayPointID scanner = WayPointID::GetIdByName(TXT($a));
        lT->AddScannerWP(scanner, TXT($a), attribs);
        qry = qm.GetQueryID((const char*) $b.text->chars);
        lT->AddWriter(wp, qry);
        // now wp is set for connList
      } connList)
    ;

scanner
    @init { string sName; // scanner name
            string rName; // relatin name
          }
    :    ^(SCANNER__ a=ID { rName=(char*)$a.text->chars; sName=rName;}
            (b=ID {sName=(char*)$b.text->chars;})?) {
            SlotContainer attribs;
            am.GetAttributesSlots(rName, attribs); // put attributes in attribs
            //WayPointID scanner = WayPointID::GetIdByName(sName.c_str());
            WayPointID scanner(sName);
            lT->AddScannerWP(scanner, rName, attribs);
        }
    ;

waypoint[bool isNew]
    :    ^(WAYPOINT__ ID {
            if (isNew){
                WayPointID nWp((const char*)$ID.text->chars);
                wp = nWp;
            } else {
                wp = WayPointID::GetIdByName((const char*)$ID.text->chars);
            }
        }  wpDefinition? bypassRule* wpbodyStatement* )
    ;

query
  :    ^(QUERRY__ (ID { qry = qm.GetQueryID((const char*) $ID.text->chars); /*qm.AddNewQuery(string((const char*) $ID.text->chars), qry);*/ })+  qBodyStatement* )
  ;


qBodyStatement
    :    ^(WAYPOINT__ ID {
            wp = WayPointID::GetIdByName((char*)$ID.text->chars);
            //wpType=typeMap[wp];
        }  bodyStatement)
    ;

wpbodyStatement
  : ^(QUERRY__ ID {qry = qm.GetQueryID((const char*) $ID.text->chars);} bodyStatement)
  ;

bodyStatement
  :    rules*
  ;

rules :
    filterRule
  | synthRule
  | aggregateRule
  | printRule
  | joinRule
  | glaRule
  ;

filterRule
    @init {SlotContainer atts; /* the set of attributes */
      string cstStr; /* the constants used in the expression */
      string defs; /* definitions needed by expressions */
        }
   : ^(FILTER expr[atts, cstStr, defs]) { lT->AddFilter(wp, qry, atts, $expr.sExpr, cstStr, defs); }
  ;

synthRule
    @init {SlotContainer atts; /* the set of attributes */
      string cstStr; /* the constants used in the expression */
      string defs; /* definitions needed by expressions */
        }
    : ^(SYNTHESIZE__ a=ID t=ID expr[atts, cstStr, defs]) {
      SlotID sID = am.AddSynthesizedAttribute(qry, (const char*)$a.text->chars,
                                                (const char*)$t.text->chars);
      lT->AddSynthesized(wp,qry, sID, atts, $expr.sExpr, cstStr, defs);
    }
  ;

aggregateRule
    @init {SlotContainer atts; /* the set of attributes */
      string cstStr; /* the constants used in the expression */
      string defs; /* definitions needed by expressions */
        }
    : ^(AGGREGATE name=ID type=ID expr[atts, cstStr, defs]) {
            SlotID aggID = am.AddSynthesizedAttribute(qry, (const char*)$name.text->chars, "DOUBLE");
            lT->AddAggregate(wp,qry, aggID, (const char*)$type.text->chars, atts, $expr.sExpr, cstStr, defs);
    }
  ;

printRule
    @init {SlotContainer atts; /* the set of attributes */
      string cstStr; /* the constants used in the expression */
      string names;
      string types;
      string file;
      string defs; /* definitions needed by expressions */
        }
    : ^(PRINT expr[atts, cstStr, defs] printAtts[names, types] printFile[file] ){ lT->AddPrint(wp, qry, atts, $expr.sExpr, cstStr, names, types, file, defs); }
  ;

printAtts[string& names, string& types]
    : /*nothing*/
    | ^(ATTWT n=ID t=ID) { names+=TXT($n); types+=TXT($t); }
      ( ^(ATTWT n=ID t=ID) { names+=","; names+=TXT($n); types+=","; types+=TXT($t); } )*
    ;

ctAttList[string& ctArgs]
    : /*no arg, nothing in result*/
    | a=ctAtt {ctArgs+=$a.s;} (b=ctAtt  {ctArgs+=", "; ctArgs+=$b.s;} )*
    ;

ctAtt returns[string s]
    : (a=STRING | a=INT | a=FLOAT) {s=(char*)($a.text->chars);}
    ;

printFile[string& s]
    : /* nothing */
    | ^(LIST a=STRING ) {s=STRS(a);}
    ;

/* accumulate arguments form GLA definitions and form m4 code to call it */
glaDef returns [string name, string defs]
  : ID {$name=(const char*) $ID.text->chars;} glaTemplate[$name,$defs]
  ;

glaTemplate[string& name, string& defs]
@init { string args; }
    : /* simpleGLA */ { defs+="m4_include("; defs+=name; defs+=".h)\n"; }
    | ^(GLATEMPLATE  ({args+=",";} glaTemplArg[args, defs] )* )
        {
         // form the template instantiation code and change name to temp
            defs+="\nm4_include(</";
         defs+=name+".h.m4/>)\n";
         string tempName = GenerateTemp("GLA_\%d");
         defs+=name;
         defs+="(";
         defs+=tempName; // args has comma
         defs+=args;
         defs+=")\n";
         name=tempName; // new name
        }
    ;

glaTemplArg[string& args, string& defs]
    : ^(LIST {args+="</";} attC[args] ({args+=",";} attC[args])* {args+="/>";})
    | attC[args] /* single typed argument */
    | GLA glaDef {
      // glue the definitions accumulated
      defs+=$glaDef.defs;
      // add the name to current definition
      args+=$glaDef.name;
    }
    | s=STRING { args+=TXTN($s); }
    | i=INT { args+=TXT($i); }
    | f=FLOAT { args+=TXT($f); }
    ;

funcTemplate[string& fName, string& defs, bool isExternal] returns [string name]
@init{ $name = fName; string args; }
    : /* nothing */
        {
            if( isExternal ) {
                defs += "#include ";
                defs += name;
                defs += ".h\n";
            }
        }
    | ^(FUNCTEMPLATE ({args += ",";} funcTemplateArg[args, defs] )* )
        {
            defs += "\nm4_include(</";
            defs += $name + ".h.m4/>)\n";
            string namePattern = "FUNC_\%d_" + $name;
            string tempName = GenerateTemp(namePattern.c_str());
            defs += $name;
            defs += "(";
            defs += tempName;
            defs += args; // args has comma
            defs += ")\n";
            $name = tempName;
        }
    ;

funcTemplateArg[string& args, string & defs]
    : ^(LIST {args += "</";} attC[args] ({args += ",";} attC[args])* {args += "/>";})
    | attC[args]
    | s=STRING { args += TXTN($s); }
    | i=INT { args += TXT($i); }
    | f=FLOAT { args += TXT($f); }
    ;

funcRetType returns [string type, bool external]
    : /* nothing */ {$external = false;}
    | ^(TYPE_ t=ID) { $type = TXT($t); $external = true; }
    ;

attWT[string& args]
    : ^(ATTWT att=ID type=ID) {
      args+="(";
      args+=(const char*) $att.text->chars;
      args+=",";
      args+=(const char*) $type.text->chars;
      args+=")";
    }
    ;

attC[string& args]
    : ^(ATTC {args+="(";}  a=ID { args+=TXT($a); }
            ( b=ID { args+=','; args+=TXT($b);} )* 
        {args+=')';})
    ;

glaRule
    @init {
            SlotContainer atts; /* the set of attributes */
            SlotContainer outAtts; /**output attributes */
            string cstStr; /* the constants used in the expression */
            string sExpr; // the entire expression representing the arguments
            ExprListInfo lInfo;
            string ctArgs="("; /* constructor arguments*/
            std::vector<std::string> outTypes;
            bool isLarge = false;
            string defs; /* the definitions needed by the expressions */
        }
    : ^(GLA (PLUS {isLarge = true;})? ctAttList[ctArgs] glaDef attLWT[outAtts, outTypes]* (a=expression[atts, cstStr, defs] {   lInfo.Add($a.sExpr, $a.type, $a.isCT); } )* ) {
        // This is we get in return
            string glaName((const char *) $glaDef.text->chars );
            // Check if operator exists
#ifdef ENFORCE_GLA_TYPES
           vector<ArgFormat> actArgs;
           if (!dTM.IsGLA(glaName, outTypes, lInfo.GetListTypes(), actArgs)) {
               printf("\nERROR: GLA \%s with arguments \%s do not exist",
                      glaName.c_str(), lInfo.GetTypesDesc().c_str());
           } else {
               // Need to tell the expression info list about the actual types of the
               // arguments and any special formatting needed using the vector of
               // ArgFormats.
               lInfo.Prepare( cstStr, actArgs );
           }
#else
           lInfo.Prepare( cstStr );
#endif
           /** in future, check if the function is pure as well */
           bool isCT = lInfo.IsListConstant();

           std::vector<string> eVals = lInfo.Generate();
           sExpr = "(";
           for (int i=0; i<eVals.size(); i++){
               if (i>0) {
                   sExpr += ",";
               }

               sExpr += eVals[i];
           }
           sExpr += ")";

           ctArgs+=")";

           defs += $glaDef.defs;

            if( isLarge )
                lT->AddGLALarge(wp, qry, outAtts, $glaDef.name, defs, ctArgs, atts, sExpr, cstStr);
            else
               lT->AddGLA(wp,qry, outAtts, $glaDef.name, defs, ctArgs, atts, sExpr, cstStr);
    }
  ;

attLWT [SlotContainer& outAtts, vector<string> &outTypes]
    : ^(ATTWT att=ID type=ID) {
            SlotID glaID = am.AddSynthesizedAttribute(qry, (const char*)$att.text->chars,
                                                      (const char*)$type.text->chars);
            string t((const char*)($type.text->chars));
            outTypes.push_back(t);
            outAtts.Append(glaID);
        }
    ;

bypassRule :
    ^(BYPASS ID) {
      qry = qm.GetQueryID((const char*) $ID.text->chars);
      lT->AddBypass(wp,qry);
    }
  ;

joinType returns [LemonTranslator::JoinType type]
: /* none, normal */ {$type = LemonTranslator::Join_EQ;}
| JOIN_IN { $type = LemonTranslator::Join_IN; }
| JOIN_NOTIN { $type = LemonTranslator::Join_NOTIN; }
;

joinRule
    @init {SlotContainer atts; /* the set of attributes */ }
    : ^(JOIN joinType attributeList[atts]) {
lT->AddJoin(wp, qry, atts, $joinType.type);
}
  ;

attribute returns [SlotID slot]
    :     att=ATT {
            $slot = am.GetAttributeSlot((char*)$att.text->chars);
            WARNINGIF( !slot.IsValid(), "Attribute does not exist");
        }
     ;

selectWP
    :    ^(SELECT__ {
            lT->AddSelectionWP(wp);
        }  connList )
    ;

joinWP
@init {SlotContainer atts;}
    : ^(JOIN attributeList[atts] { lT->AddJoinWP(wp, atts); } connList)
    ;

attributeList[SlotContainer& atts]
: ^(ATTS (a=attribute {$atts.Append($a.slot);})+ )
;

aggregateWP
  :    ^(AGGREGATE {
        lT->AddAggregateWP(wp);
    }
      connList )
  ;

glaWP
    @init {
        bool isLarge = false;
    }
    : ^(GLA (PLUS {isLarge = true;})? {
        if( isLarge )
            lT->AddGLALargeWP(wp);
        else
            lT->AddGLAWP(wp);
    }
      connList )
  ;

printWP
    :    ^(PRINT {
      lT->AddPrintWP(wp);
      }
      connList )
  ;

connList
    : wayPointCN+
    ;

wayPointCN
    :    ID {
          WayPointID nWP = WayPointID::GetIdByName((char*)$ID.text->chars);
          lT->AddEdge(nWP, wp);
        }
    | TERMCONN ID{
          WayPointID nWP = WayPointID::GetIdByName((char*)$ID.text->chars);
          lT->AddTerminatingEdge(nWP, wp);
        }
    ;

textloaderWP
@init { SlotContainer attribs; char sep; string tablePattern; int count = 0;}
@after { lT->AddTextLoaderWP(wp, attribs, sep, attribs, tablePattern, count); }
  :
    ^(TEXTLOADER__ (
        ^(ATTFROM ID) { am.GetAttributesSlots(TXT($ID), attribs);}
  | ^(SEPARATOR a=STRING) {sep = char(*(StripQuotes(TXT($a)).c_str()));}
  | ^(FILE__ b=STRING { tablePattern = TXTS($b); count=1; } (INT {count= atoi(TXT($INT));})? )
  )+ )
;

wpDefinition
  : selectWP
    | joinWP
    | aggregateWP
    | printWP
    | textloaderWP
    | glaWP
  ;

expr[SlotContainer& atts, string& cstStr, string& defs] returns [string sExpr] :
    (a=expression[atts, cstStr, defs] {sExpr+=$a.sExpr;} )
    (b=expression[atts, cstStr, defs] {sExpr+=", "+$b.sExpr;} ) *
    | ^('?' a=expression[atts, cstStr, defs] {sExpr+=$a.sExpr;} b=expression[atts, cstStr, defs] {sExpr+="?";sExpr+=$b.sExpr;} c=expression[atts, cstStr, defs] {sExpr+=":";sExpr+=$c.sExpr;})
    ;

// There could be more than one consts, hence we must have cstArray or something alike
expression[SlotContainer& atts, string& cstStr, string& defs] returns [string sExpr, string type, bool isCT]
@init { ExprListInfo lInfo; string rType; string fName; }
  :
    ^(OPERATOR a=expression[atts, cstStr, defs] { lInfo.Add($a.sExpr, $a.type, $a.isCT); }
      b=expression[atts, cstStr, defs] { lInfo.Add($b.sExpr, $b.type, $b.isCT); } ) // binary
  {
      string funcName((char*)($OPERATOR.text->chars));
      bool funcPure = true;
      // This is we get in return
      // Check if operator exists
#ifdef ENFORCE_TYPES
      vector<ArgFormat> actArgs;
      if (!dTM.IsFunction(funcName, lInfo.GetListTypes(), $type, funcPure, actArgs)) {
        printf("\nERROR: Operator \%s with arguments \%s do not exist",
               funcName.c_str(), lInfo.GetTypesDesc().c_str());
      }
      else {
          // Need to tell the expression info list about the actual types of the
          // arguments and any special formatting needed using the vector of
          // ArgFormats.
          lInfo.Prepare( $cstStr, actArgs );
      }
#else
      lInfo.Prepare( $cstStr );
#endif

      $isCT = lInfo.IsListConstant() && funcPure;

      std::vector<string> eVals = lInfo.Generate();
      $sExpr = "(";
      $sExpr += eVals[0];
      $sExpr += funcName;
      $sExpr += eVals[1];
      $sExpr += ")";
    }
  | ^(UOPERATOR a=expression[atts, cstStr, defs]) // unary
    {
      lInfo.Add($a.sExpr, $a.type, $a.isCT);

      string funcName((char*)($UOPERATOR.text->chars));
      bool funcPure = true;
      // This is we get in return
      // Check if operator exists
#ifdef ENFORCE_TYPES
      vector<ArgFormat> actArgs;
      if (!dTM.IsFunction(funcName, lInfo.GetListTypes(), $type, funcPure, actArgs)) {
        printf("\nERROR: Operator \%s with arguments \%s do not exist",
               funcName.c_str(), lInfo.GetTypesDesc().c_str());
      }
      else {
          // Need to tell the expression info list about the actual types of the
          // arguments and any special formatting needed using the vector of
          // ArgFormats.
          lInfo.Prepare( $cstStr, actArgs );
      }
#else
      lInfo.Prepare( $cstStr );
#endif
      $isCT = lInfo.IsListConstant() && funcPure;

      std::vector<string> eVals = lInfo.Generate();
      $sExpr = "(";
      $sExpr += funcName.c_str();
      $sExpr += eVals[0];
      $sExpr += ")";
    }

  | ^(FUNCTION i=ID {fName = TXT($i);} rt=funcRetType t=funcTemplate[fName, defs, $rt.external]  (a=expression[atts, cstStr, defs] {   lInfo.Add($a.sExpr, $a.type, $a.isCT); } )*) // Function
    {
      string funcName((char*)($i.text->chars));
      bool funcPure = true;
      // This is we get in return
      // Check if operator exists
#ifdef ENFORCE_TYPES
      vector<ArgFormat> actArgs;
      if ( !$rt.external && !dTM.IsFunction(funcName, lInfo.GetListTypes(), $type, funcPure, actArgs)) {
        printf("\nERROR: Operator \%s with arguments \%s do not exist",
               funcName.c_str(), lInfo.GetTypesDesc().c_str());
      }

      // Need to tell the expression info list about the actual types of the
      // arguments and any special formatting needed using the vector of
      // ArgFormats.
      lInfo.Prepare( $cstStr, actArgs );
#else
      lInfo.Prepare( $cstStr );
#endif
      $isCT = lInfo.IsListConstant() && funcPure;

      if( $rt.external )
        $type = $rt.type;

      std::vector<string> eVals = lInfo.Generate();
      $sExpr = $t.name;
      $sExpr += "(";
      for (int i=0; i<eVals.size(); i++){
        if (i>0) {
          $sExpr += ",";
        }

        $sExpr += eVals[i];
      }
      $sExpr += ")";
}
| ^(MATCH_DP patt=STRING a=expression[atts, cstStr, defs] { lInfo.Add($a.sExpr, $a.type, $a.isCT); } ) // pattern matcher
    {
        // for a pattern matcher, we have to build an expression
        // of the form: PatternMather ctObj(pattern)
        // then on use do ctObj.IsMatch(expr)
        $type = "bool";

        $isCT = lInfo.IsListConstant();

        lInfo.Prepare( $cstStr );
        std::vector<string> eVals = lInfo.Generate();
        // new constant
        int ctNo = ExprListInfo::NextVar();
        // add def of matcher object
        ostringstream match;
        match << "PatternMatcher ct" << ctNo << "( string("
              <<  TXTN($patt) << ") );" << endl;

        ADD_CST($cstStr, match.str());
        // now the expression
        ostringstream expr;
        expr << "ct" << ctNo << ".IsMatch(" << eVals[0] << ")";
        $sExpr+=expr.str();
   }
| ^(CASE_DP (a=expression[atts, cstStr, defs] {   lInfo.Add($a.sExpr, $a.type, $a.isCT); } )*) // cases
   {
        // just like a function. For now we only support the 3-argument case
        $isCT = lInfo.IsListConstant();

        lInfo.Prepare( $cstStr );
        std::vector<string> eVals = lInfo.Generate();
        FATALIF(eVals.size()!=3, "We only support CASE(test, true_expr, false_expr) for now");
        $sExpr = $sExpr+"("+eVals[0]+") ? ("+eVals[1]+") : ("+eVals[2]+")";
   }
| att=ATT  // Attribute
{
        string longName = (char*)$att.text->chars;
        SlotID slot = am.GetAttributeSlot(longName.c_str());
        FATALIF( !slot.IsValid(), "Attribute does not exist, how did this happen?");
        atts.Append(slot);
        // Add the attribute long name string to the expression
        $sExpr = longName;
        // Find the correct function to get attr type
        $type = dTM.GetBaseType(am.GetAttributeType(longName.c_str()));
        $isCT = false;
}
| INT
{
    string sExpr_text((char*)($INT.text->chars));
    $sExpr = sExpr_text;
    if( sExpr_text[sExpr_text.size()-1] == 'L') // Literal long int
        $type = "BIGINT";
    else
        $type = "INT";          // Literal int
    $isCT = true;
}
| BOOL_T
{
    $sExpr = (char*)($BOOL_T.text->chars);
    $type = "bool";
    $isCT = true;
}
|    STRING
{
    $sExpr = TXTN($STRING);
    $type = "STRING_LITERAL";
    $isCT = true;
}
|    FLOAT
{
    string sExpr_text((char*) ($FLOAT.text->chars));
    $sExpr = (char*)($FLOAT.text->chars);
    if( sExpr_text[sExpr_text.size()-1] == 'L' )    // Literal long double
        $type = "DOUBLE";
    else if( sExpr_text[sExpr_text.size() -1] == 'f' )  // Literal float
        $type = "FLOAT";
    else    // Literal double
        $type = "DOUBLE";
    $isCT = true;
}
;

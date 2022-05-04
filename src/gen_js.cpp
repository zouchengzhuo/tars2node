/**
 * Tencent is pleased to support the open source community by making Tars available.
 *
 * Copyright (C) 2016THL A29 Limited, a Tencent company. All rights reserved.
 *
 * Licensed under the BSD 3-Clause License (the "License"); you may not use this file except 
 * in compliance with the License. You may obtain a copy of the License at
 *
 * https://opensource.org/licenses/BSD-3-Clause
 *
 * Unless required by applicable law or agreed to in writing, software distributed 
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR 
 * CONDITIONS OF ANY KIND, either express or implied. See the License for the 
 * specific language governing permissions and limitations under the License.
 */

#include "code_generator.h"

/**
 * @brief 生成枚举字符串
 * 
 * @param pPtr 枚举指针
 * @param sNamespace 所属namespace
 * @return string 一个枚举的字符串
 */
string CodeGenerator::generateJS(const EnumPtr &pPtr, const string &sNamespace)
{
	ostringstream s;
    bool bDependent = false;

    if (_bEnumReverseMappings)
    {
        s << TAB << "(function(" << pPtr->getId() << ") {" << endl;
        INC_TAB;

        int nenum = -1;
        vector<TypeIdPtr>& member = pPtr->getAllMemberPtr();
        for (size_t i = 0; i < member.size(); i++)
        {
            bDependent |= isDependent(sNamespace, member[i]->getId());

            if (member[i]->hasDefault())
            {
                nenum = TC_Common::strto<int>(member[i]->def());
            }
            else
            {
                nenum++;
            }

            s << TAB << pPtr->getId() << "[" << pPtr->getId() << "[\"" << member[i]->getId() << "\"] = " << TC_Common::tostr(nenum) << "] = "
              << "\"" << member[i]->getId() << "\";" << endl;
        }
        DEL_TAB;
        s << TAB << "}(" << sNamespace << "." << pPtr->getId() << " = {}));" << endl;
    }
    else
    {
        s << TAB << sNamespace << "." << pPtr->getId() << " = {" << endl;
        INC_TAB;

        int nenum = -1;
        vector<TypeIdPtr>& member = pPtr->getAllMemberPtr();
        for (size_t i = 0; i < member.size(); i++)
        {
            bDependent |= isDependent(sNamespace, member[i]->getId());

            if (member[i]->hasDefault())
            {
                nenum = TC_Common::strto<int>(member[i]->def());
            }
            else
            {
                nenum++;
            }

            s << TAB << "\"" << member[i]->getId() << "\" : " << TC_Common::tostr(nenum) << (i < member.size() - 1 ? "," : "") << endl;
        }
        DEL_TAB;
        s << TAB << "};" << endl;
    }

    s << TAB << sNamespace << "." << pPtr->getId() << "._classname = \"" << sNamespace << "." << pPtr->getId() << "\";" << endl;
    s << TAB << sNamespace << "." << pPtr->getId() << "._write = function(os, tag, val) { return os.writeInt32(tag, val); };" << endl;
    s << TAB << sNamespace << "." << pPtr->getId() << "._read  = function(is, tag, def) { return is.readInt32(tag, true, def); };" << endl;

    if (!_bMinimalMembers || _bEntry || bDependent || isDependent(sNamespace, pPtr->getId())) {
        return s.str();
    } else {
        return "";
    }
}

/**
 * @brief 生成常量
 * 
 * @param pPtr 常量指针
 * @param sNamespace 所属namespace
 * @param bNeedStream out参数，需要用二进制表示字符串时标记为 true
 * @return string 一个常量的字符串
 */
string CodeGenerator::generateJS(const ConstPtr &pPtr, const string &sNamespace, bool &bNeedStream)
{
    if (_bMinimalMembers && !_bEntry && !isDependent(sNamespace, pPtr->getTypeIdPtr()->getId()))
    {
        return "";
    }

    ostringstream s;
    
    if (_bStringBinaryEncoding && GET_CONST_GRAMMAR_PTR(pPtr)->t == CONST_GRAMMAR(STRING))
    {
        bNeedStream = true;
    }

    s << TAB << sNamespace << "." << pPtr->getTypeIdPtr()->getId() << " = " << getDefault(pPtr->getTypeIdPtr(), GET_CONST_GRAMMAR_PTR(pPtr)->v, sNamespace, false) << ";" << endl;

    return s.str();
}

/**
 * @brief 生成一个 struct 的字符串
 * 
 * @param pPtr struct 指针
 * @param sNamespace 所属的 namespace
 * @param bNeedAssert out参数，标记struct未实现 _equal 函数所需的 key 操作
 * @param bQuickFunc out参数，标记使用了函数名引用，如：var _hasOwnProperty = Object.prototype.hasOwnProperty;
 * @return string 一个结构体的字符串
 */
string CodeGenerator::generateJS(const StructPtr &pPtr, const string &sNamespace, bool &bNeedAssert, bool &bQuickFunc)
{
    if (_bMinimalMembers && !_bEntry && !isDependent(sNamespace, pPtr->getId()))
    {
        return "";
    }

    ostringstream s;
    vector<TypeIdPtr>& member = pPtr->getAllMemberPtr();

    s << TAB << sNamespace << "." << pPtr->getId() << " = function() {"<< endl;

    INC_TAB;

    for (size_t i = 0; i < member.size(); i++)
    {
        s << TAB << "this." << member[i]->getId() << " = " << getDefault(member[i], member[i]->def(), sNamespace) << ";" << endl;
    }

    {
        s << TAB << "this._classname = \"" << sNamespace << "." << pPtr->getId() << "\";" << endl; 
    }

    DEL_TAB;

    s << TAB << "};" << endl;
    s << TAB << sNamespace << "." << pPtr->getId() << "._classname = \"" << sNamespace << "." << pPtr->getId() << "\";" << endl; 

	string sProto = TC_Common::replace(pPtr->getSid(), "::", ".");
	s << TAB << sProto << "._write = function (os, tag, value) { os.writeStruct(tag, value); };" << endl;
	s << TAB << sProto << "._read  = function (is, tag, def) { return is.readStruct(tag, true, def); };" << endl;

    //_readFrom
    s << TAB << sNamespace << "." << pPtr->getId() << "._readFrom = function (is) {" << endl;
    INC_TAB;
    s << TAB << "var tmp = new " << sNamespace << "." << pPtr->getId() << ";" << endl;
    for (size_t i = 0; i < member.size(); i++)
    {
        string sFuncName = toFunctionName(member[i], "read");
		s << TAB << "tmp." << member[i]->getId() << " = is." << sFuncName << "(" << member[i]->getTag()
            << ", " << (member[i]->isRequire()?"true":"false") << ", ";

        if (isSimple(member[i]->getTypePtr()))
        {
            s << getDefault(member[i], member[i]->def(), sNamespace)
                << representArgument(member[i]->getTypePtr());
        }
        else
        {
            s << getDataType(member[i]->getTypePtr());
        }

        s << ");" << endl;
    }
    s << TAB << "return tmp;" << endl;
    DEL_TAB;
    s << TAB << "};" << endl;

    //_writeTo
    s << TAB << sNamespace << "." << pPtr->getId() << ".prototype._writeTo = function (os) {" << endl;

    INC_TAB;
    for (size_t i = 0; i < member.size(); i++)
    {
        string sFuncName = toFunctionName(member[i], "write");

       	s << TAB << "os." << sFuncName << "(" << member[i]->getTag() << ", this." << member[i]->getId()
            << representArgument(member[i]->getTypePtr()) << ");" << endl;
    }
    DEL_TAB;
    s << TAB << "};" << endl;

    /*
     *  Size Optimize:
     *    Remove <mutil_map> support.
     *    Remove toBinBuffer, readFromObject, toObject, new, create members.
     */ 
    if (_iOptimizeLevel == Os)
    {
        return s.str();
    }

    //_equal
    vector<string> key = pPtr->getKey();
    
    s << TAB << sNamespace << "." << pPtr->getId() << ".prototype._equal = function (" << (key.size() > 0 ? "anItem" : "") << ") {" << endl;
    
    INC_TAB;

    if (key.size() > 0)
    {
        s << TAB << "return ";

        for (size_t i = 0; i < key.size(); i++)
        {
            for (size_t ii =0; ii < member.size(); ii++)
            {
                if (key[i] != member[ii]->getId())
                {
                    continue;
                }

                if (isSimple(member[i]->getTypePtr()))
                {
                    s << (i==0?"":TAB + TAB) << "this." << key[i] << " === " << "anItem." << key[i];
                }
                else 
                {
                    s << (i==0?"":TAB + TAB) << "this._equal(" << "anItem)";
                }
            }

            if (i != key.size() - 1) 
            {
                s << " && " << endl;
            }
        }

        s << ";" << endl;

    }
    else 
    {
        bNeedAssert = true;
        s << TAB << "assert.fail(\"this structure not define key operation\");" << endl;
    }

    DEL_TAB;
    s << TAB << "};" << endl;

    //_genKey
    s << TAB << sNamespace << "." << pPtr->getId() << ".prototype._genKey = function () {" << endl;
    INC_TAB;
    s << TAB << "if (!this._proto_struct_name_) {" << endl;
    INC_TAB;
    s << TAB << "this._proto_struct_name_ = \"STRUCT\" + Math.random();" << endl;
    DEL_TAB;
    s << TAB << "}" << endl;
    s << TAB << "return this._proto_struct_name_;" << endl;
    DEL_TAB;
    s << TAB << "};" << endl;

    //toObject
    s << TAB << sNamespace << "." << pPtr->getId() << ".prototype.toObject = function() { "<< endl;
    INC_TAB;
    s << TAB << "return {" << endl;

    for (size_t i = 0; i < member.size(); i++)
    {
        INC_TAB;
        if (i > 0 && i < member.size()) {
            s << "," << endl;
        }

        if (isSimple(member[i]->getTypePtr())) {
            s << TAB << "\"" << member[i]->getId() << "\" : this." << member[i]->getId();
        }
        else {
            s << TAB << "\"" << member[i]->getId() << "\" : this." << member[i]->getId() << ".toObject()";
        }
        DEL_TAB;
    }

    s << endl;
    s << TAB << "};" << endl;
    DEL_TAB;
    s << TAB << "};" << endl;

    //readFromObject
    s << TAB << sNamespace << "." << pPtr->getId() << ".prototype.readFromObject = function(json) { "<< endl;
    INC_TAB;

    for (size_t i = 0; i < member.size(); i++)
    {
        if (isSimple(member[i]->getTypePtr())) {
            s << TAB << "_hasOwnProperty.call(json, \"" << member[i]->getId() << "\") && (this." << member[i]->getId() << " = json." << member[i]->getId() << ");" << endl;
        } else {
            s << TAB << "_hasOwnProperty.call(json, \"" << member[i]->getId() << "\") && (this." << member[i]->getId() << ".readFromObject(json." << member[i]->getId() << "));" << endl;
        }
        bQuickFunc = true;
    }

    s << TAB << "return this;" << endl;

    DEL_TAB;
    s << TAB << "};" << endl;

    //toBinBuffer
    s << TAB << sNamespace << "." << pPtr->getId() << ".prototype.toBinBuffer = function () {" << endl;
    INC_TAB;
    s << TAB << "var os = new " << IDL_NAMESPACE_STR << "Stream." << IDL_TYPE << "OutputStream();" << endl;
    s << TAB << "this._writeTo(os);" << endl;
    s << TAB << "return os.getBinBuffer();" << endl;
    DEL_TAB;
    s << TAB << "};" << endl;

    //new
    s << TAB << sNamespace << "." << pPtr->getId() << ".new = function () {" << endl;
    INC_TAB;
    s << TAB << "return new " << sNamespace << "." << pPtr->getId() << "();" << endl;
    DEL_TAB;
    s << TAB << "};" << endl;

    //create
    s << TAB << sNamespace << "." << pPtr->getId() << ".create = function (is) {" << endl;
    INC_TAB;
    s << TAB << "return " << sNamespace << "." << pPtr->getId() << "._readFrom(is);" << endl;
    DEL_TAB;
    s << TAB << "};" << endl;

    return s.str();
}

/**
 * @brief 生成一个 namespace 下所有结构体、枚举、常量 的字符串
 * 
 * @param pPtr namespace 指针
 * @param bNeedStream out参数， 当常量字符串不为空时，设置为true
 * @param bNeedAssert out参数，生成的js代码需要 assert时设置为true
 * @param bQuickFunc out参数，使用了快捷函数时设置为true
 * @return string 一个 namespace 下，所有结构体、枚举、常量 的字符串
 */
string CodeGenerator::generateJS(const NamespacePtr &pPtr, bool &bNeedStream, bool &bNeedAssert, bool &bQuickFunc)
{
    // 存放所有的结构体字符串
    ostringstream sstr;
    vector<StructPtr> ss(pPtr->getAllStructPtr());
    // 遍历并处理结构体
    for (size_t last = 0; last != ss.size() && ss.size() != 0;)
    {
        last = ss.size();

        for (vector<StructPtr>::iterator iter=ss.begin(); iter!=ss.end();)
        {
            string str = generateJS(*iter, pPtr->getId(), bNeedAssert, bQuickFunc);

            if (!str.empty()) {
                sstr << str << endl;
                iter = ss.erase(iter);
                
            } else {
                iter++;
            }
        }
    }

    // 存放所有的常量字符串
    ostringstream cstr;
	vector<ConstPtr> &cs = pPtr->getAllConstPtr();
    // 遍历并处理常量
	for (size_t i = 0; i < cs.size(); i++)
	{
        string str = generateJS(cs[i], pPtr->getId(), bNeedStream);

        if (!str.empty()) {
            cstr << str << endl;
        }
	}
    // 存放枚举字符串
    ostringstream estr;
    // 遍历并处理枚举
	vector<EnumPtr> & es = pPtr->getAllEnumPtr();
    for (size_t i = 0; i < es.size(); i++)
    {
        string str = generateJS(es[i], pPtr->getId());
        
        if (!str.empty()) {
            estr << str << endl;
        }
    }
    //拼接字符串并返回
    ostringstream str;
    if (!estr.str().empty()) str << estr.str();
    if (!cstr.str().empty()) str << cstr.str();
    if (!sstr.str().empty())
    {
        bNeedStream = true;
        str << sstr.str();
    }

	return str.str();
}

/**
 * @brief 生成整个 context 的字符串。 这个是生成 XXXXTars.js 时用的，非client proxy代码
 * 
 * @param pPtr 
 * @return true 有 结构体/枚举/常量
 * @return false 无 结构体/枚举/常量
 */
bool CodeGenerator::generateJS(const ContextPtr &pPtr)
{
    vector<NamespacePtr> namespaces = pPtr->getNamespaces();

    ostringstream istr;
    set<string> setNamespace;
    // 遍历并处理 namespace，生成 namespace 的声明和 export 语句
    for(size_t i = 0; i < namespaces.size(); i++)
    {
        if (setNamespace.count(namespaces[i]->getId()) == 0)
        {
            istr << TAB << "var " << namespaces[i]->getId() << " = " << namespaces[i]->getId() << " || {};" << endl;
            istr << TAB << "module.exports." << namespaces[i]->getId() << " = " << namespaces[i]->getId() << ";" << endl << endl;

            setNamespace.insert(namespaces[i]->getId());
        }
    }

    // generate encoders and decoders
    ostringstream estr;
    bool bNeedAssert = false;
    bool bNeedStream = false;
    bool bQuickFunc = false;
    // 遍历并处理 namespace，生成 结构体、枚举、常量的字符串
    for(size_t i = 0; i < namespaces.size(); i++)
    {
        estr << generateJS(namespaces[i], bNeedStream, bNeedAssert, bQuickFunc);
    }
    // 生成后的结构体、枚举、常量 最终返回是空字符，返回false
    if (estr.str().empty())
    {
        return false;
    }

    // generate module imports
    // 生成模块 import 语句
    ostringstream ostr;
    for (map<string, ImportFile>::iterator it = _mapFiles.begin(); it != _mapFiles.end(); it++)
    {
        if (it->second.sModule.empty()) continue;

        if (estr.str().find(it->second.sModule + ".") == string::npos) continue;

        ostr << "var " << it->second.sModule << " = require(\"" << it->second.sFile << "\");" << endl;
    }

    // concat generated code
    // 组装最终生成的语句
    ostringstream sstr;
    sstr << printHeaderRemark("Structure");
    sstr << DISABLE_ESLINT << endl;
    sstr << endl;
    sstr << "\"use strict\";" << endl << endl;
    if (bNeedAssert)
    {
        sstr << "var assert    = require(\"assert\");" << endl;
    }
    if (bNeedStream)
    {
        sstr << "var " << IDL_NAMESPACE_STR << "Stream = require(\"" << _sStreamPath << "\");" << endl;
    }
    if (bQuickFunc)
    {
        sstr << endl;
        sstr << "var _hasOwnProperty = Object.prototype.hasOwnProperty;" << endl;
    }
    sstr << ostr.str() << endl;
    sstr << istr.str();
    sstr << estr.str() << endl;

    string sFileName = TC_File::excludeFileExt(_sToPath + TC_File::extractFileName(pPtr->getFileName())) + IDL_TYPE + ".js";
    TC_File::makeDirRecursive(_sToPath);
    makeUTF8File(sFileName, sstr.str());

    return true;
}
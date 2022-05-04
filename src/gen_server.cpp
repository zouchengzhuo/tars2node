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

string CodeGenerator::generatePing(const NamespacePtr &nPtr, const InterfacePtr &pPtr)
{
    ostringstream str;

    str << TAB << "var __" << nPtr->getId() << "_" << pPtr->getId() << "$" << TC_Common::lower(IDL_NAMESPACE_STR) << "_ping$RE = function (_ret) {" << endl;
    INC_TAB;
    str << TAB << "if (this.getRequestVersion() === " << PROTOCOL_SIMPLE << " || this.getRequestVersion() === " << PROTOCOL_COMPLEX << ") {" << endl;
    INC_TAB;
    str << TAB << "var " << PROTOCOL_VAR << " = new " << IDL_NAMESPACE_STR << "Stream.UniAttribute();" << endl; 
    str << TAB << PROTOCOL_VAR << "." << PROTOCOL_VAR << "Version = this.getRequestVersion();" << endl; 
    str << TAB << PROTOCOL_VAR << ".writeInt32(\"\", _ret);" << endl << endl; 
    str << TAB << "this.doResponse(" << PROTOCOL_VAR << ".encode());" << endl;
    DEL_TAB;
    str << TAB << "} else {" << endl;
    INC_TAB;
    str << TAB << "var os = new " << IDL_NAMESPACE_STR << "Stream." << IDL_TYPE << "OutputStream();" << endl; 
    str << TAB << "os.writeInt32(0, _ret);" << endl << endl; 
    str << TAB << "this.doResponse(os.getBinBuffer());" << endl;
    DEL_TAB;
    str << TAB << "}" << endl;
    DEL_TAB;
    str << TAB << "};" << endl << endl;

    str << TAB << nPtr->getId() << "." << pPtr->getId() << "Imp.prototype.__" << TC_Common::lower(IDL_NAMESPACE_STR) << "_ping = function (current) {" << endl;
    INC_TAB;
    str << TAB << "__" << nPtr->getId() << "_" << pPtr->getId() << "$" << TC_Common::lower(IDL_NAMESPACE_STR) << "_ping$RE.call(current, 0);" << endl << endl;
    str << TAB << "return " << IDL_NAMESPACE_STR << "Error.SUCCESS;" << endl;
    DEL_TAB;
    str << TAB << "};" << endl;

    return str.str();
}

/**
 * @brief 异步响应函数代码实现，处理给客户端发送响应结果的过程
 * 
 * @param nPtr 
 * @param pPtr 
 * @param oPtr 
 * @return string 异步响应函数代码实现，如 var __EDU_EduOOV$getOOVPhone$RE = function (_ret, stRsp) {...}
 */
string CodeGenerator::generateAsync(const NamespacePtr &nPtr, const InterfacePtr &pPtr, const OperationPtr &oPtr)
{
    ostringstream str;

    // 生成参数列表
    string sParams = "";
    if (oPtr->getReturnPtr()->getTypePtr())
    {
        sParams += "_ret";

        // push the symbol into dependent list
        getDataType(oPtr->getReturnPtr()->getTypePtr());
    }

    vector<ParamDeclPtr> & vParamDecl = oPtr->getAllParamDeclPtr();
    for (size_t i = 0; i < vParamDecl.size(); i++)
    {
        if (!vParamDecl[i]->isOut()) continue;

        sParams += (sParams.empty()?"":", ") + vParamDecl[i]->getTypeIdPtr()->getId();
    }
    // 函数定义
    str << TAB << "var __" << nPtr->getId() << "_" << pPtr->getId() << "$" << oPtr->getId() << "$RE = function (" << sParams << ") {" << endl;

    INC_TAB;
    // 参数为空的响应
    if (sParams.empty())
    {
        str << TAB << "this.doResponse(new " << IDL_NAMESPACE_STR << "Stream.BinBuffer());" << endl;
        DEL_TAB;
        str << TAB << "};" << endl;

        return str.str();
    }
    // wup协议的响应发送代码
    str << TAB << "if (this.getRequestVersion() === " << PROTOCOL_SIMPLE << " || this.getRequestVersion() === " << PROTOCOL_COMPLEX << ") {" << endl;
    INC_TAB;
    str << TAB << "var " << PROTOCOL_VAR << " = new " << IDL_NAMESPACE_STR << "Stream.UniAttribute();" << endl;
    str << TAB << PROTOCOL_VAR << "." << PROTOCOL_VAR << "Version = this.getRequestVersion();" << endl;
    if (oPtr->getReturnPtr()->getTypePtr())
    {
        str << TAB << PROTOCOL_VAR << "." << toFunctionName(oPtr->getReturnPtr(), "write") << "(\"\", _ret"
            << representArgument(oPtr->getReturnPtr()->getTypePtr()) << ");" << endl;
    }
    for (size_t i = 0; i < vParamDecl.size(); i++)
    {
        if (!vParamDecl[i]->isOut()) continue;

        str << TAB << PROTOCOL_VAR << "." << toFunctionName(vParamDecl[i]->getTypeIdPtr(), "write") << "(\"" 
            << vParamDecl[i]->getTypeIdPtr()->getId() << "\", " << vParamDecl[i]->getTypeIdPtr()->getId()
            << representArgument(vParamDecl[i]->getTypeIdPtr()->getTypePtr()) << ");" << endl;
    }
    str << endl;
    str << TAB << "this.doResponse(" << PROTOCOL_VAR << ".encode());" << endl;
    DEL_TAB;

    //// ========= 增加对 JSON_VERSION 支持
    str << TAB << "} else if (this.getRequestVersion() === " << PROTOCOL_JSON << ") {" << endl;

    INC_TAB;
    str << TAB << "var _data_ = {};" << endl;
    if (oPtr->getReturnPtr()->getTypePtr())
    {
        str << TAB << "_data_[\"tars_ret\"] = _ret;" << endl;
    }
    for (size_t i = 0; i < vParamDecl.size(); i++)
    {
        if (!vParamDecl[i]->isOut())
            continue;

        str << TAB << "_data_[\"" << vParamDecl[i]->getTypeIdPtr()->getId() << "\"] = " << vParamDecl[i]->getTypeIdPtr()->getId()
            << ".toObject ? " << vParamDecl[i]->getTypeIdPtr()->getId() << ".toObject() : "
            << vParamDecl[i]->getTypeIdPtr()->getId() << ";" << endl;
    }
    str << endl;
    str << TAB << " this.doResponse(new TarsStream.BinBuffer(Buffer.from(JSON.stringify(_data_))));" << endl;
    DEL_TAB;
    //// ========= 普通 tars 协议的响应

    str << TAB << "} else {" << endl;

    INC_TAB;
    str << TAB << "var os = new " << IDL_NAMESPACE_STR << "Stream." << IDL_TYPE << "OutputStream();" << endl;
    if (oPtr->getReturnPtr()->getTypePtr())
    {
        str << TAB << "os." << toFunctionName(oPtr->getReturnPtr(), "write") << "(0, _ret"
            << representArgument(oPtr->getReturnPtr()->getTypePtr()) << ");" << endl;
    }
    for (size_t i = 0; i < vParamDecl.size(); i++)
    {
        if (!vParamDecl[i]->isOut()) continue;

        str << TAB << "os." << toFunctionName(vParamDecl[i]->getTypeIdPtr(), "write") << "(" 
            << (i + 1) << ", " << vParamDecl[i]->getTypeIdPtr()->getId()
            << representArgument(vParamDecl[i]->getTypeIdPtr()->getTypePtr()) << ");" << endl;
    }
    str << endl;
    str << TAB << "this.doResponse(os.getBinBuffer());" << endl;
    DEL_TAB;
    str << TAB << "}" << endl;

    DEL_TAB;

    str << TAB << "};" << endl;

    return str.str();
}

/**
 * @brief onDispatch 中要调用的 真正的 dispatch 函数的实现，真正处理请求的函数
 * 
 * @param nPtr 
 * @param pPtr 
 * @param oPtr 
 * @return string 真正的 dispatch 函数的实现，如 EDU.EduOOVImp.prototype.__getOOVTrans = function (current, binBuffer) {...}
 */
string CodeGenerator::generateDispatch(const NamespacePtr &nPtr, const InterfacePtr &pPtr, const OperationPtr &oPtr)
{
    ostringstream str;
    vector<ParamDeclPtr> & vParamDecl = oPtr->getAllParamDeclPtr();
    // 函数定义
    str << TAB << nPtr->getId() << "." << pPtr->getId() << "Imp.prototype.__" << oPtr->getId() << " = function (current" << (vParamDecl.size() != 0 ? ", binBuffer" : "") << ") {" << endl;

    INC_TAB;

    ostringstream dstr;

    for (size_t i = 0; i < vParamDecl.size(); i++)
    {
        dstr << TAB << "var " << vParamDecl[i]->getTypeIdPtr()->getId() << " = null;" << endl;
    }
    if (vParamDecl.size() != 0)
    {
        dstr << endl;
    }
    // wup 协议处理
    dstr << TAB << "if (current.getRequestVersion() === " << PROTOCOL_SIMPLE << " || current.getRequestVersion() === " << PROTOCOL_COMPLEX << ") {" << endl;
    INC_TAB;
    dstr << TAB << "var " << PROTOCOL_VAR << " = new " << IDL_NAMESPACE_STR << "Stream.UniAttribute();" << endl;
    dstr << TAB << PROTOCOL_VAR << "." << PROTOCOL_VAR << "Version = current.getRequestVersion();" << endl;
    dstr << TAB << PROTOCOL_VAR << ".decode(binBuffer);" << endl;

    for (size_t i = 0; i < vParamDecl.size(); i++)
    {
        dstr << TAB << vParamDecl[i]->getTypeIdPtr()->getId()
                << " = " << PROTOCOL_VAR << "." << toFunctionName(vParamDecl[i]->getTypeIdPtr(), "read")
                << "(\"" << vParamDecl[i]->getTypeIdPtr()->getId() << "\"";

        if (!isSimple(vParamDecl[i]->getTypeIdPtr()->getTypePtr()) && !isBinBuffer(vParamDecl[i]->getTypeIdPtr()->getTypePtr()))
        {
            dstr << ", " << getDataType(vParamDecl[i]->getTypeIdPtr()->getTypePtr());
        }

        if (vParamDecl[i]->isOut())
        {
            dstr << ", " << getDefault(vParamDecl[i]->getTypeIdPtr(), "", nPtr->getId(), true)
                    << representArgument(vParamDecl[i]->getTypeIdPtr()->getTypePtr());
        }
        
        dstr << ");" << endl;
    }
    DEL_TAB;

    //// ========= 增加对 JSON_VERSION 支持
    dstr << TAB << "} else if (current.getRequestVersion() === " << PROTOCOL_JSON << ") {" << endl;
    INC_TAB;
    dstr << TAB << "var _data_ = JSON.parse(binBuffer.toNodeBuffer());" << endl;

    for (size_t i = 0; i < vParamDecl.size(); i++)
    {
        if (vParamDecl[i]->isOut())
        {
            //taf_data.rsp ? rsp.readFromObject(taf_data.rsp) : rsp;
            StructPtr sPtr = StructPtr::dynamicCast(vParamDecl[i]->getTypeIdPtr()->getTypePtr());
            if (sPtr)
            {
                dstr << TAB << vParamDecl[i]->getTypeIdPtr()->getId()
                    << " = " << getDefault(vParamDecl[i]->getTypeIdPtr(), "", nPtr->getId(), true)
                    << ";" << endl;
                dstr << TAB << "_data_." << vParamDecl[i]->getTypeIdPtr()->getId() << " ? " << vParamDecl[i]->getTypeIdPtr()->getId() << ".readFromObject("
                    << "_data_." << vParamDecl[i]->getTypeIdPtr()->getId() << ") : " << vParamDecl[i]->getTypeIdPtr()->getId()
                    << ";" << endl;
            }
            else
            {
                dstr << TAB << vParamDecl[i]->getTypeIdPtr()->getId()
                 << " = _data_." << vParamDecl[i]->getTypeIdPtr()->getId() << " || " << getDefault(vParamDecl[i]->getTypeIdPtr(), "", nPtr->getId(), true)
                 << ";" << endl;
            }
            // dstr << TAB << vParamDecl[i]->getTypeIdPtr()->getId()
            //      << " = _data_." << vParamDecl[i]->getTypeIdPtr()->getId() << " || " << getDefault(vParamDecl[i]->getTypeIdPtr(), "", nPtr->getId(), true)
            //      << ";" << endl;
        }
        else
        {
            // dstr << TAB << vParamDecl[i]->getTypeIdPtr()->getId()
            //      << " = _data_." << vParamDecl[i]->getTypeIdPtr()->getId()
            //      << ";" << endl;
            StructPtr sPtr = StructPtr::dynamicCast(vParamDecl[i]->getTypeIdPtr()->getTypePtr());
            if (sPtr)
            {
                dstr << TAB << vParamDecl[i]->getTypeIdPtr()->getId()
                    << " = " << getDefault(vParamDecl[i]->getTypeIdPtr(), "", nPtr->getId(), true)
                    << ";" << endl;
                dstr << TAB << vParamDecl[i]->getTypeIdPtr()->getId() << ".readFromObject("
                    << "_data_." << vParamDecl[i]->getTypeIdPtr()->getId() << ")"
                    << ";" << endl;
            }
            else
            {
                dstr << TAB << vParamDecl[i]->getTypeIdPtr()->getId()
				<< " = _data_." << vParamDecl[i]->getTypeIdPtr()->getId() 
				<< ";" << endl;
            }
        }
    }
    DEL_TAB;
    //// ========= 普通 tars 协议请求的处理

    dstr << TAB << "} else {" << endl;

    INC_TAB;
    dstr << TAB << "var is = new " << IDL_NAMESPACE_STR << "Stream." << IDL_TYPE << "InputStream(binBuffer);" << endl;
    string sParams = "";
    for (size_t i = 0; i < vParamDecl.size(); i++)
    {
        sParams += ", " + vParamDecl[i]->getTypeIdPtr()->getId();

        dstr << TAB << vParamDecl[i]->getTypeIdPtr()->getId()
                << " = is." << toFunctionName(vParamDecl[i]->getTypeIdPtr(), "read") << "("
                << (i + 1) << ", " << (vParamDecl[i]->isOut() ? "false" : "true") << ", ";

        if (isSimple(vParamDecl[i]->getTypeIdPtr()->getTypePtr()))
        {
            dstr << getDefault(vParamDecl[i]->getTypeIdPtr(), vParamDecl[i]->getTypeIdPtr()->def(), nPtr->getId())
                    << representArgument(vParamDecl[i]->getTypeIdPtr()->getTypePtr());
        }
        else 
        {
            dstr << getDataType(vParamDecl[i]->getTypeIdPtr()->getTypePtr());
        }

        dstr << ");" << endl;
    }
    DEL_TAB;
    dstr << TAB << "}" << endl << endl;

    if (!sParams.empty())
    {
        str << dstr.str();
    }

    str << TAB << "current.sendResponse = __" << nPtr->getId() << "_" << pPtr->getId() << "$" << oPtr->getId() << "$RE;" << endl << endl;

    str << TAB << "this." << oPtr->getId() << "(current" << sParams << ");" << endl << endl;

    str << TAB << "return " << IDL_NAMESPACE_STR << "Error.SUCCESS;" << endl;

    DEL_TAB;

    str << TAB << "};" << endl;

    return str.str();
}

/**
 * @brief 生成业务函数体（实际上是基类实现，内容只有一个 assert.fail，需要用户重写 imp 函数业务处理逻辑）
 * 
 * @param nPtr 
 * @param pPtr 
 * @param oPtr 
 * @return string 基类的业务函数体
 */
string CodeGenerator::generateJSServer(const NamespacePtr &nPtr, const InterfacePtr &pPtr, const OperationPtr &oPtr)
{
    ostringstream str;

    str << TAB << nPtr->getId() << "." << pPtr->getId() << "Imp.prototype." << oPtr->getId() << " = function () {" << endl;

    INC_TAB;
    str << TAB << "assert.fail(\"" << oPtr->getId() << " function not implemented\");" << endl;
    DEL_TAB;
    str << TAB << "};" << endl;

    return str.str();
}

/**
 * @brief 生成一个 interface 的 servant 的代码
 * 
 * @param pPtr 
 * @param nPtr 
 * @return string 一个 interface servant 实现代码
 */
string CodeGenerator::generateJSServer(const InterfacePtr &pPtr, const NamespacePtr &nPtr)
{
    ostringstream str;
    vector<OperationPtr> & vOperation = pPtr->getAllOperationPtr();

    // generate the implementation class
    // 生成 imp 函数定义
    str << TAB << nPtr->getId() << "." << pPtr->getId() << "Imp = function () { " << endl;
    INC_TAB;
    str << TAB << "this._name   = undefined;" << endl;
    str << TAB << "this._worker = undefined;" << endl;
    DEL_TAB;
    str << TAB << "};" << endl << endl;

    // generate the initialize function
    // 生成 initialize 函数
    str << TAB << nPtr->getId() << "." << pPtr->getId() << "Imp.prototype.initialize = function () {};" << endl << endl;

    // generate the dispatch function
    // 生成 onDispatch 函数
    str << TAB << nPtr->getId() << "." << pPtr->getId() << "Imp.prototype.onDispatch = function (current, funcName, binBuffer) { " << endl;
    INC_TAB;
    str << TAB << "if (\"__\" + funcName in this) {" << endl;
    INC_TAB;
    str << TAB << "return this[\"__\" + funcName](current, binBuffer);" << endl;
    DEL_TAB;
    str << TAB << "} else {" << endl;
    INC_TAB;
    str << TAB << "return " << IDL_NAMESPACE_STR << "Error.SERVER.FUNC_NOT_FOUND;" << endl;
    DEL_TAB;
    str << TAB << "}" << endl;
    DEL_TAB;
    str << TAB << "};" << endl << endl;

    // generate the ping function
    // 生成 ping 函数
    str << generatePing(nPtr, pPtr) << endl;
    // generate functions
    // 生成所有业务函数
    for (size_t i = 0; i < vOperation.size(); i++)
    {
        str << generateJSServer(nPtr, pPtr, vOperation[i]) << endl;
        str << generateAsync(nPtr, pPtr, vOperation[i]) << endl;
        str << generateDispatch(nPtr, pPtr, vOperation[i]) << endl;
    }

    str << endl;

    return str.str();
}

/**
 * @brief 生成namespace 中 所有 interface 的 servant 的代码
 * 
 * @param pPtr 
 * @param bNeedStream out参数，是否依赖 @tars/stream 模块
 * @param bNeedRpc out参数，是否依赖 @tars/rpc 模块
 * @param bNeedAssert out参数，是否依赖 assert 函数
 * @return string namespace 中 所有interface 的 servant 实现代码
 */
string CodeGenerator::generateJSServer(const NamespacePtr &pPtr, bool &bNeedStream, bool &bNeedRpc, bool &bNeedAssert)
{
	ostringstream str;

    vector<InterfacePtr> & is = pPtr->getAllInterfacePtr();
    for (size_t i = 0; i < is.size(); i++)
    {
        str << generateJSServer(is[i], pPtr) << endl;
    }
    if (is.size() != 0)
    {
        bNeedRpc = true;
        bNeedStream = true;
        bNeedAssert = true;
    }

	return str.str();
}

/**
 * @brief 生成一个 服务端 imp 的完整字符串，含结构体、枚举、常量 以及 interface servant实现
 * 
 * @param pPtr 
 * @return true 
 * @return false 
 */
bool CodeGenerator::generateJSServer(const ContextPtr &pPtr)
{
    vector<NamespacePtr> namespaces = pPtr->getNamespaces();

    ostringstream istr;
    set<string> setNamespace;
    // 遍历 context 中的 namespace， 生成 模块定义 和 export 语句
    for(size_t i = 0; i < namespaces.size(); i++)
    {
        if (setNamespace.count(namespaces[i]->getId()) == 0)
        {
            istr << TAB << "var " << namespaces[i]->getId() << " = " << namespaces[i]->getId() << " || {};" << endl;
            istr << TAB << "module.exports." << namespaces[i]->getId() << " = " << namespaces[i]->getId() << ";" << endl << endl;

            setNamespace.insert(namespaces[i]->getId());
        }
    }

    // generate server classes with encoders and decoders
    ostringstream estr;
    bool bNeedAssert = false;
    bool bNeedStream = false;
    bool bQuickFunc = false;
    // 遍历namespace，生成结构体、枚举、常量的字符串
    for(size_t i = 0; i < namespaces.size(); i++)
    {
        estr << generateJS(namespaces[i], bNeedStream, bNeedAssert, bQuickFunc);
    }

    bool bNeedRpc = false;
    //遍历namespace，生成 interface 和 其中所有方法的代码
    for(size_t i = 0; i < namespaces.size(); i++)
    {
        estr << generateJSServer(namespaces[i], bNeedStream, bNeedRpc, bNeedAssert) << endl;
    }

    if (estr.str().empty())
    {
        return false;
    }

    // generate module imports
    // 生成依赖模块的引入
    ostringstream ostr;
    if (bNeedAssert)
    {
        ostr << TAB << "var assert    = require(\"assert\");" << endl;
    }
    if (bNeedStream)
    {
        ostr << TAB << "var " << IDL_NAMESPACE_STR << "Stream = require(\"" << _sStreamPath << "\");" << endl;
    }
    if (bNeedRpc)
    {
        ostr << TAB << "var " << IDL_NAMESPACE_STR << "Error  = require(\"" << _sRpcPath << "\").error;" << endl;
    }
    // 生成依赖协议的import语句
    for (map<string, ImportFile>::iterator it = _mapFiles.begin(); it != _mapFiles.end(); it++)
    {
        if (it->second.sModule.empty()) continue;

        if (estr.str().find(it->second.sModule + ".") == string::npos) continue;

        ostr << TAB << "var " << it->second.sModule << " = require(\"" << it->second.sFile << "\");" << endl;
    }
    if (bQuickFunc)
    {
        ostr << endl;
        ostr << TAB << "var _hasOwnProperty = Object.prototype.hasOwnProperty;" << endl;
    }

    ostringstream str;
    // 组装、生成最终代码  XXXX.js
    str << printHeaderRemark("Server");
    str << DISABLE_ESLINT << endl;
    str << endl;
    str << "\"use strict\";" << endl << endl;
    str << ostr.str() << endl;
    str << istr.str();
    str << estr.str() << endl;

    string sFileName = TC_File::excludeFileExt(_sToPath + TC_File::extractFileName(pPtr->getFileName())) + ".js";

    TC_File::makeDirRecursive(_sToPath);
    makeUTF8File(sFileName, str.str());

    return true;
}
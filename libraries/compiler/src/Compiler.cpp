////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Machine Learning Library (EMLL)
//  File:     Compiler.cpp (compiler)
//  Authors:  Umesh Madan
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Compiler.h"
#include "CompilerException.h"

namespace emll
{
	namespace compiler
	{
		const std::string c_InputVariableName = "input";
		const std::string c_OutputVariableName = "output";
		const std::string c_PredictFunctionName = "Predict";

		/// <summary>Base class for ML Compiler.</summary>
		Compiler::Compiler()
			: _nodeTypes()
		{
			InitSupportedNodeTypes();
		}

		const std::string& Compiler::InputName() const
		{
			return c_InputVariableName;
		}

		const std::string& Compiler::OutputName() const
		{
			return c_OutputVariableName;
		}

		TempVar Compiler::AllocTemp()
		{
			return _tempVars.Alloc();
		}

		void Compiler::FreeTemp(TempVar var)
		{
			_tempVars.Free(var);
		}

		uint64_t Compiler::AllocGlobal()
		{
			_globalVars++;
			return _globalVars;
		}

		void Compiler::CompileModel(const model::Model& model)
		{
			_inputs = ModelEx::CollectInputNodes(model);
			_outputs = ModelEx::CollectOutputNodes(model);
			BeginMain(c_PredictFunctionName);
			EndMain();
		}

		void Compiler::CompileNode(DataNode& node)
		{
			BeginMain(c_PredictFunctionName);
			switch (node.Type())
			{
				case DataNodeType::Literal:
					Compile(static_cast<LiteralNode&>(node));
					break;
				default:
					throw new CompilerException(CompilerError::notSupported);
			}
			EndMain();
		}

		Compiler::NodeType Compiler::GetNodeType(const model::Node& node) const
		{
			return _nodeTypes.Get(node.GetRuntimeTypeName());
		}

		void Compiler::VerifyInputType(const model::Node& node, const model::Port::PortType type)
		{
			auto inputs = node.GetInputPorts();
			for (auto& port : inputs)
			{
				if (port->GetType() != type)
				{
					throw new CompilerException(CompilerError::inputPortTypeNotSupported);
				}
			}
		}

		void Compiler::VerifyOutputType(const model::Node& node, const model::Port::PortType type)
		{
			auto outputs = node.GetOutputPorts();
			for (auto& port : outputs)
			{
				if (port->GetType() != type)
				{
					throw new CompilerException(CompilerError::outputPortTypeNotSupported);
				}
			}
		}

		void Compiler::InitSupportedNodeTypes()
		{
			_nodeTypes.Init	({
				{"Input", NodeType::input},
				{"ConstantNode", NodeType::constant},
				{"BinaryOperationNode", NodeType::binaryOp}
			});
		}

		void Compiler::Reset()
		{
		}

		model::Port::PortType ModelEx::GetNodeDataType(const model::Node& node)
		{
			return node.GetOutputPorts()[0]->GetType();
		}

		std::vector<const model::Node*> ModelEx::CollectOutputNodes(const model::Model& model)
		{
			auto findNodes = [](const model::Node& node)
			{
				return (typeid(node) == typeid(model::InputNode<double>) ||
					typeid(node) == typeid(model::InputNode<int>));
			};
			return CollectNodes(model, [](const model::Node& node) { return IsLeafNode(node); });
		}

		std::vector<const model::Node*> ModelEx::CollectInputNodes(const model::Model& model)
		{
			auto findNodes = [](const model::Node& node)
			{
				return (typeid(node) == typeid(model::InputNode<double>) ||
						typeid(node) == typeid(model::InputNode<int>));
			};
			return CollectNodes(model, findNodes);
		}

		std::vector<const model::Node*> ModelEx::CollectNodes(const model::Model& model, std::function<bool(const model::Node& node)> predicate)
		{
			std::vector<const model::Node*> matches;
			model.Visit([&matches, &predicate](const model::Node& node) {
				if (predicate(node))
				{
					matches.push_back(&node);
				}
			});
			return matches;
		}


		size_t ModelEx::CountOutputs(std::vector<const model::Node*>& nodes)
		{
			size_t count = 0;
			for (auto n : nodes)
			{
				count += n->GetOutputPorts().size();
			}
			return count;
		}

		size_t ModelEx::CountInputs(std::vector<const model::Node*>& nodes)
		{
			size_t count = 0;
			for (auto n : nodes)
			{
				count += n->GetInputPorts().size();
			}
			return count;
		}

		bool ModelEx::IsLeafNode(const model::Node& node)
		{
			return (node.GetDependentNodes().size() == 0);
		}
	}
}
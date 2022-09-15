#include "statement.h"

#include <cassert>
#include <iostream>
#include <sstream>

using namespace std;

namespace ast {

using runtime::Closure;
using runtime::Context;
using runtime::ObjectHolder;

namespace {
const string ADD_METHOD = "__add__"s;
const string INIT_METHOD = "__init__"s;
}  // namespace

ObjectHolder Assignment::Execute(Closure& closure, Context& context) {
    closure[var_] = rv_->Execute(closure, context);
    return closure[var_];
}

Assignment::Assignment(std::string var, std::unique_ptr<Statement> rv)
    :var_(std::move(var))
    ,rv_(std::move(rv))
{
}

VariableValue::VariableValue(const std::string& var_name)
    :var_name_(std::move(var_name))
{

}

VariableValue::VariableValue(std::vector<std::string> dotted_ids)
    :dotted_ids_(std::move(dotted_ids))
{

}

ObjectHolder VariableValue::Execute(Closure& closure, Context& /*context*/) {
    using runtime::ClassInstance;
    if(var_name_) {
        if(closure.count(var_name_.value())) {
            return closure.at(var_name_.value());
        }
    } else if (dotted_ids_) {    
        if(dotted_ids_.value().size() > 0
           && closure.count(dotted_ids_.value().front()))
        {
            ObjectHolder obj_current = closure.at(dotted_ids_.value().front());
            ClassInstance* cl_i = obj_current.TryAs<ClassInstance>();
            if( !cl_i && dotted_ids_.value().size() == 1) {
                return obj_current;
            }
            for (size_t i = 1; i < dotted_ids_.value().size();
                 ++i, cl_i = obj_current.TryAs<ClassInstance>() )
            {
                obj_current = cl_i->Fields().at(dotted_ids_.value()[i]);
            }
            return obj_current;
        }
    }
    throw std::runtime_error("value undefined"s);
//    return {};
}

unique_ptr<Print> Print::Variable(const std::string& name) {
    return make_unique<Print>(make_unique<VariableValue>(name));
//    throw std::logic_error("Not implemented"s);
}

Print::Print(unique_ptr<Statement> argument)
    :argument_(std::move(argument))
{

}

Print::Print(vector<unique_ptr<Statement>> args)
    :args_(std::move(args))
{

}

ObjectHolder Print::Execute(Closure& closure, Context& context) {
    if(argument_) {
        runtime::Object* obj = argument_.value()->Execute(closure, context).Get();
        if(obj) {
            obj->Print(context.GetOutputStream(), context);
        } else {
            context.GetOutputStream() << "None";
        }
        context.GetOutputStream() << endl;
        return argument_.value()->Execute(closure, context);
    } else if(args_) {
        bool flg_no_first = false;
        for(std::unique_ptr<Statement>& statm : args_.value()) {
            runtime::ObjectHolder obj = statm->Execute(closure, context);
            if (flg_no_first) {
                context.GetOutputStream() << " ";
            }
            if(obj) {
                obj->Print(context.GetOutputStream(), context);
            } else {
                context.GetOutputStream() << "None";
            }
            flg_no_first = true;
        }
        context.GetOutputStream() << endl;
    }
    return {};
}

MethodCall::MethodCall(std::unique_ptr<Statement> object, std::string method,
                       std::vector<std::unique_ptr<Statement>> args)
    :object_(std::move(object))
    ,method_(std::move(method))
    ,args_(std::move(args))
{

}

ObjectHolder MethodCall::Execute(Closure& closure, Context& context) {
    runtime::ClassInstance* cls_i = object_->Execute(closure, context).TryAs<runtime::ClassInstance>();
    if(cls_i) {
        std::vector<ObjectHolder> actual_args;
        actual_args.reserve(args_.size());
        for(auto& arg : args_) {
            actual_args.push_back(arg->Execute(closure, context));
        }
        return cls_i->Call(method_, actual_args, context);
    }
    return {};
}

ObjectHolder Stringify::Execute(Closure& closure, Context& context) {
    runtime::DummyContext cntxt;
    //runtime::Object* obj = argument_->Execute(closure, context).Get();
    ObjectHolder obj = argument_->Execute(closure, context);
    if(obj) {
        obj->Print(cntxt.GetOutputStream(), cntxt);
    } else {
        cntxt.output << "None"s;
    }
    return ObjectHolder::Own<runtime::String>(cntxt.output.str());
}

ObjectHolder Add::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs = lhs_->Execute(closure, context);
    ObjectHolder rhs = rhs_->Execute(closure, context);

    if(lhs.TryAs<runtime::Number>()
       && rhs.TryAs<runtime::Number>()) {
       return  ObjectHolder::Own<runtime::Number>(lhs.TryAs<runtime::Number>()->GetValue() +
               rhs.TryAs<runtime::Number>()->GetValue());
    }
    if(lhs.TryAs<runtime::String>()
       && rhs.TryAs<runtime::String>()) {
       return  ObjectHolder::Own<runtime::String>(lhs.TryAs<runtime::String>()->GetValue() +
               rhs.TryAs<runtime::String>()->GetValue());
    }
    if(runtime::ClassInstance* cl_i = lhs.TryAs<runtime::ClassInstance>();
       cl_i)
    {
       if (cl_i->HasMethod(ADD_METHOD, 1 )) {
           return cl_i->Call(ADD_METHOD, {rhs}, context);
       }
    }
    throw std::runtime_error("Add unsuccess!");
}

ObjectHolder Sub::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs = lhs_->Execute(closure, context);
    ObjectHolder rhs = rhs_->Execute(closure, context);

    if(lhs.TryAs<runtime::Number>()
       && rhs.TryAs<runtime::Number>()) {
       return  ObjectHolder::Own<runtime::Number>(lhs.TryAs<runtime::Number>()->GetValue() -
               rhs.TryAs<runtime::Number>()->GetValue());
    }
    throw std::runtime_error("Sub unsuccess!");
}

ObjectHolder Mult::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs = lhs_->Execute(closure, context);
    ObjectHolder rhs = rhs_->Execute(closure, context);

    if(lhs.TryAs<runtime::Number>()
       && rhs.TryAs<runtime::Number>()) {
       return  ObjectHolder::Own<runtime::Number>(lhs.TryAs<runtime::Number>()->GetValue() *
               rhs.TryAs<runtime::Number>()->GetValue());
    }
    throw std::runtime_error("Mult unsuccess!");
}

ObjectHolder Div::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs = lhs_->Execute(closure, context);
    ObjectHolder rhs = rhs_->Execute(closure, context);

    if(lhs.TryAs<runtime::Number>()
       && rhs.TryAs<runtime::Number>()
       && rhs.TryAs<runtime::Number>()->GetValue() != 0) {
       return  ObjectHolder::Own<runtime::Number>(lhs.TryAs<runtime::Number>()->GetValue() /
               rhs.TryAs<runtime::Number>()->GetValue() );
    }
    ostringstream in;
    in << lhs.TryAs<runtime::Number>()->GetValue() << " / ";
    in << rhs.TryAs<runtime::Number>()->GetValue();
    throw std::runtime_error("Div unsuccess! " + in.str());
}

ObjectHolder Compound::Execute(Closure& closure, Context& context) {
    for(auto& stmt : stmts_) {
        stmt->Execute(closure, context);
    }
    return ObjectHolder::None();
}

ObjectHolder Return::Execute(Closure& closure, Context& context) {
    throw ReturnException(statement_->Execute(closure, context));
}

ClassDefinition::ClassDefinition(ObjectHolder cls)
    :cls_(cls)
{
   assert(cls_.Get() == cls.Get());
}

ObjectHolder ClassDefinition::Execute(Closure& closure, Context& /*context*/) {
    runtime::Class* cls = cls_.TryAs<runtime::Class>();
    std::string name = cls->GetName();
    closure[name] = ObjectHolder::Share(*cls);
    return closure[std::move(name)];
}

FieldAssignment::FieldAssignment(VariableValue object, std::string field_name,
                                 std::unique_ptr<Statement> rv)
    :object_(std::move(object))
    ,field_name_(std::move(field_name))
    ,rv_(std::move(rv))
{
}

ObjectHolder FieldAssignment::Execute(Closure& closure, Context& context) {
    // находим в closure объект по имени которое хранится в object_
    ObjectHolder obj_in_colosure = object_.Execute(closure, context);
    if( runtime::ClassInstance* cl_i = obj_in_colosure.TryAs<runtime::ClassInstance>();
        cl_i)
    {
        // добавляем поле с именем и значение типа ObjectHolder
        cl_i->Fields()[field_name_] = rv_->Execute(closure, context);
        return cl_i->Fields().at(field_name_);
    }
    return {};
}

IfElse::IfElse(std::unique_ptr<Statement> condition, std::unique_ptr<Statement> if_body,
               std::unique_ptr<Statement> else_body)
    :condition_(std::move(condition))
    ,if_body_(std::move(if_body))
    ,else_body_(std::move(else_body))
{

}

ObjectHolder IfElse::Execute(Closure& closure, Context& context) {
    if(runtime::IsTrue(condition_->Execute(closure, context))) {
       return if_body_->Execute(closure, context);
    }
    if(else_body_) {
        return else_body_->Execute(closure, context);
    }
    return {};
}

ObjectHolder Or::Execute(Closure& closure, Context& context) {
    if(IsTrue(lhs_->Execute(closure, context))) {
        return ObjectHolder::Own(runtime::Bool(true));
    }
    if(IsTrue(rhs_->Execute(closure, context))) {
        return ObjectHolder::Own(runtime::Bool(true));
    }
    return ObjectHolder::Own(runtime::Bool(false));
}

ObjectHolder And::Execute(Closure& closure, Context& context) {
    if(IsTrue(lhs_->Execute(closure, context))) {
        if(IsTrue(rhs_->Execute(closure, context))) {
            return ObjectHolder::Own(runtime::Bool(true));
        }
    }
    return ObjectHolder::Own(runtime::Bool(false));
}

ObjectHolder Not::Execute(Closure& closure, Context& context) {
    return ObjectHolder::Own(runtime::Bool(!IsTrue(argument_->Execute(closure, context))));
}

Comparison::Comparison(Comparator cmp, unique_ptr<Statement> lhs, unique_ptr<Statement> rhs)
    : BinaryOperation(std::move(lhs), std::move(rhs))
    ,cmp_(cmp)
{
}

ObjectHolder Comparison::Execute(Closure& closure, Context& context) {
    ObjectHolder lhs = lhs_->Execute(closure, context);
    ObjectHolder rhs = rhs_->Execute(closure, context);

    return ObjectHolder::Own<runtime::Bool>(cmp_(lhs, rhs, context));
}

NewInstance::NewInstance(const runtime::Class& class_, std::vector<std::unique_ptr<Statement>> args)
    :cls_(&class_)
    ,args_(std::move(args)) {
}

NewInstance::NewInstance(const runtime::Class& class_)
    :cls_(&class_) {
}

ObjectHolder NewInstance::Execute(Closure& closure, Context& context) {
    ObjectHolder obj_cls_i = ObjectHolder::Own(runtime::ClassInstance(*cls_));
    runtime::ClassInstance* cls_i = obj_cls_i.TryAs<runtime::ClassInstance>();
    // если был вызов без параметров
    if(!args_) {
        // если конструктор без параметров
        if(cls_i->HasMethod(INIT_METHOD, 0)) {
            cls_i->Call(INIT_METHOD, {}, context);
        }
        return obj_cls_i;//ObjectHolder::Own(std::move(cl_i_));
    }
    // если у класса есть метод init и колличество аргументов совпадает
    else if(cls_i->HasMethod(INIT_METHOD, args_->size())) {
        std::vector<ObjectHolder> actual_args;
        actual_args.reserve(args_->size());
        for(auto& arg : args_.value()) {
            actual_args.push_back(arg->Execute(closure, context));
        }
        cls_i->Call(INIT_METHOD, actual_args, context);
        //return ObjectHolder::Own(std::move(cl_i_));
        //return ObjectHolder::Share(cl_i_);
        return obj_cls_i;
    }
    // инече возвращаем класс без инициализации полей
    //return ObjectHolder::Own(std::move(cl_i_));
    return obj_cls_i;
}

MethodBody::MethodBody(std::unique_ptr<Statement>&& body)
    :body_(std::move(body)){
}

ObjectHolder MethodBody::Execute(Closure& closure, Context& context) {
    try {
        body_->Execute(closure, context);
    } catch (ReturnException& r) {
        return r.obj_;
    }
    return ObjectHolder::None();
}

}  // namespace ast

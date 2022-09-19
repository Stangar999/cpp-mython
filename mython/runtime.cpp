#include "runtime.h"

#include <algorithm>
#include <cassert>
#include <optional>
#include <sstream>

using namespace std;

namespace runtime {

ObjectHolder::ObjectHolder(std::shared_ptr<Object> data)
    : data_(std::move(data)) {
}

void ObjectHolder::AssertIsValid() const {
    assert(data_ != nullptr);
}

ObjectHolder ObjectHolder::Share(Object& object) {
    // Возвращаем невладеющий shared_ptr (его deleter ничего не делает)
    return ObjectHolder(std::shared_ptr<Object>(&object, [](auto* /*p*/) { /* do nothing */ }));
}

ObjectHolder ObjectHolder::None() {
    return ObjectHolder();
}

Object& ObjectHolder::operator*() const {
    AssertIsValid();
    return *Get();
}

Object* ObjectHolder::operator->() const {
    AssertIsValid();
    return Get();
}

Object* ObjectHolder::Get() const {
    return data_.get();
}

ObjectHolder::operator bool() const {
    return Get() != nullptr;
}

bool IsTrue(const ObjectHolder& object) {
    if(!object
       || (object.TryAs<Bool>() && object.TryAs<Bool>()->GetValue() == false)
       || (object.TryAs<String>() && object.TryAs<String>()->GetValue() == ""s)
       || (object.TryAs<Number>() && object.TryAs<Number>()->GetValue() == 0)
       || object.TryAs<Class>()
       || object.TryAs<ClassInstance>()) {
        return false;
    }
    return true;
}

void ClassInstance::Print(std::ostream& os, Context& context) {
    if(const Method* metod = cls_->GetMethod(STR_METHOD); metod != nullptr ) {
        this->Call(STR_METHOD, {}, context)->Print(os, context);
    } else {
        os << this;
    }
}

bool ClassInstance::HasMethod(const std::string& method, size_t argument_count) const {
    if(const Method* mtd = cls_->GetMethod(method);
       mtd != nullptr && mtd->formal_params.size() == argument_count ) {
        return true;
    }
    return false;
}

Closure& ClassInstance::Fields() {
    return closure_;
    throw std::logic_error("Not implemented"s);
}

const Closure& ClassInstance::Fields() const {
    return closure_;
    throw std::logic_error("Not implemented"s);
}

ClassInstance::ClassInstance(const Class& cls)
    :cls_(&cls)
{}

ObjectHolder ClassInstance::Call(const std::string& method,
                                 const std::vector<ObjectHolder>& actual_args,
                                 Context& context) {

    const Method* mtd = cls_->GetMethod(method);
    if(mtd == nullptr || mtd->formal_params.size() != actual_args.size()) {
        throw std::runtime_error("Not implemented"s);
    }
    Closure closure;
    closure["self"s] = ObjectHolder::Share(*this);
    for(size_t i = 0; i < mtd->formal_params.size(); ++i) {
        closure[mtd->formal_params[i]] = actual_args[i];
    }
    return mtd->body->Execute(closure, context);
}

Class::Class(std::string name, std::vector<Method> methods, const Class* parent)
    :name_(std::move(name))
    ,methods_(std::move(methods))
    ,parent_(parent)
{}

const Method* Class::GetMethod(const std::string& name) const {
    auto eqv = [&name](const Method& lth){
        return lth.name == name;
    };
    auto result = find_if(methods_.begin(), methods_.end(), eqv);
    if (result != methods_.end()) {
        return &(*result);
    }
    if(!parent_) {
        return nullptr;
    }
    result = find_if(parent_->methods_.begin(), parent_->methods_.end(), eqv);
    if (result != parent_->methods_.end()) {
        return &(*result);
    }
    return nullptr;
}

[[nodiscard]] const std::string& Class::GetName() const {
    return name_;
}

void Class::Print(ostream& os, Context& /*context*/) {
    os << "Class " << name_;
}

void Bool::Print(std::ostream& os, [[maybe_unused]] Context& context) {
    os << (GetValue() ? "True"sv : "False"sv);
}

bool Equal(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    if(!lhs && !rhs ) {
        return true;
    }
    return Compare(lhs, rhs, context, std::equal_to(), "__eq__"s);
}

bool Less(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    return Compare(lhs, rhs, context, std::less(), "__lt__"s);
}

bool NotEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    try {
        return !Equal(lhs, rhs, context);
    } catch (...){
        throw std::runtime_error("Cannot compare objects for NotEqual"s);
    }
}

bool Greater(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    try {
        return !Less(lhs, rhs, context) && !Equal(lhs, rhs, context);
    } catch (...){
        throw std::runtime_error("Cannot compare objects for Greater"s);
    }
}

bool LessOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    try {
        return !Greater(lhs, rhs, context);
    } catch (...){
        throw std::runtime_error("Cannot compare objects for LessOrEqual"s);
    }
}

bool GreaterOrEqual(const ObjectHolder& lhs, const ObjectHolder& rhs, Context& context) {
    try {
        return !Less(lhs, rhs, context);
    } catch (...){
        throw std::runtime_error("Cannot compare objects for GreaterOrEqual"s);
    }
}

}  // namespace runtime

#include "./Vst3PluginFactory.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <vector>

#include "./pluginterfaces/base/ftypes.h"
#include "./Vst3Utils.hpp"
#include "./Vst3Plugin.hpp"
#include "./Module.hpp"
#include "./StrCnv.hpp"

#include "debugger_output.hpp"
using namespace Steinberg;

namespace hwm {

extern
std::unique_ptr<Vst3Plugin>
	CreatePlugin(IPluginFactory *factory, ClassInfo const &info, Vst3PluginFactory::host_context_type host_context);

FactoryInfo::FactoryInfo(PFactoryInfo const &info)
    :	vendor_(hwm::to_wstr(info.vendor))
	,	url_(hwm::to_wstr(info.url))
	,	email_(hwm::to_wstr(info.email))
	,	flags_(info.flags)
{}

bool FactoryInfo::discardable				() const
{
	return (flags_ & Steinberg::PFactoryInfo::FactoryFlags::kClassesDiscardable) != 0; 
}

bool FactoryInfo::license_check				() const
{
	return (flags_ & Steinberg::PFactoryInfo::FactoryFlags::kLicenseCheck) != 0; 
}

bool FactoryInfo::component_non_discardable	() const
{
	return (flags_ & Steinberg::PFactoryInfo::FactoryFlags::kComponentNonDiscardable) != 0; 
}

bool FactoryInfo::unicode					() const
{
	return (flags_ & Steinberg::PFactoryInfo::FactoryFlags::kUnicode) != 0; 
}

String	FactoryInfo::vendor	() const
{
	return vendor_;
}

String	FactoryInfo::url		() const
{
	return url_;
}

String	FactoryInfo::email	() const
{
	return email_;
}

ClassInfo2Data::ClassInfo2Data(Steinberg::PClassInfo2 const &info)
	:	sub_categories_(hwm::to_wstr(info.subCategories))
	,	vendor_(hwm::to_wstr(info.vendor))
	,	version_(hwm::to_wstr(info.version))
	,	sdk_version_(hwm::to_wstr(info.sdkVersion))
{
}

ClassInfo2Data::ClassInfo2Data(Steinberg::PClassInfoW const &info)
	:	sub_categories_(hwm::to_wstr(info.subCategories))
	,	vendor_(hwm::to_wstr(info.vendor))
	,	version_(hwm::to_wstr(info.version))
	,	sdk_version_(hwm::to_wstr(info.sdkVersion))
{
}

ClassInfo::ClassInfo(Steinberg::PClassInfo const &info)
	:	cid_()
	,	name_(hwm::to_wstr(info.name))
	,	category_(hwm::to_wstr(info.category))
	,	cardinality_(info.cardinality)
{
    std::copy(std::begin(info.cid), std::end(info.cid), cid_.begin());
}

ClassInfo::ClassInfo(Steinberg::PClassInfo2 const &info)
	:	cid_()
	,	name_(hwm::to_wstr(info.name))
	,	category_(hwm::to_wstr(info.category))
	,	cardinality_(info.cardinality)
	,	classinfo2_data_(info)
{
	std::copy(std::begin(info.cid), std::end(info.cid), cid_.begin());
}

ClassInfo::ClassInfo(Steinberg::PClassInfoW const &info)
	:	cid_()
	,	name_(hwm::to_wstr(info.name))
	,	category_(hwm::to_wstr(info.category))
	,	cardinality_(info.cardinality)
	,	classinfo2_data_(info)
{
	std::copy(std::begin(info.cid), std::end(info.cid), cid_.begin());
}

struct Vst3PluginFactory::Impl
{
	Impl(String module_path);
	~Impl();

	size_t	GetComponentCount() { return class_info_list_.size(); }
	ClassInfo const &
			GetComponent(size_t index) { return class_info_list_[index]; }

	FactoryInfo const &
			GetFactoryInfo() { return factory_info_; }

	IPluginFactory *
			GetFactory()
	{
		return factory_.get();
	}

private:
	Module module_;
	typedef void (*SetupProc)();

	typedef std::unique_ptr<IPluginFactory, SelfReleaser> factory_ptr;

	factory_ptr				factory_;
	FactoryInfo				factory_info_;
	std::vector<ClassInfo>	class_info_list_;
};

String FormatCid(Steinberg::int8 const *cid_array)
{
	int const reg_str_len = 48; // including null-terminator
	std::string reg_str(reg_str_len, '\0');
    FUID fuid;
    fuid.fromTUID(cid_array);
    fuid.toRegistryString(&reg_str[0]);
    return hwm::to_wstr(reg_str.c_str());
}

std::wstring
		ClassInfoToString(ClassInfo const &info)
{
	std::wstringstream ss;
	ss	<< info.name()
		<< L", " << FormatCid(info.cid())
		<< L", " << info.category()
		<< L", " << info.cardinality();

	if(info.is_classinfo2_enabled()) {
		ss	<< L", " << info.classinfo2().sub_categories()
			<< L", " << info.classinfo2().vendor()
			<< L", " << info.classinfo2().version()
			<< L", " << info.classinfo2().sdk_version()
			;
	}
	return ss.str();
}

template<class Container>
void OutputClassInfoList(Container const &cont)
{
	hwm::wdout << "--- Output Class Info ---" << std::endl;
	int count = 0;
	for(ClassInfo const &info: cont) {
		hwm::wdout
			<< L"[" << count++ << L"] " << ClassInfoToString(info) << std::endl;
	}
}

std::wstring
		FactoryInfoToString(FactoryInfo const &info)
{
	std::wstringstream ss;
	ss	<< info.vendor()
		<< L", " << info.url()
		<< L", " << info.email()
		<< std::boolalpha
		<< L", " << L"Discardable: " << info.discardable()
		<< L", " << L"License Check: "<< info.license_check()
		<< L", " << L"Component Non Discardable: " << info.component_non_discardable()
		<< L", " << L"Unicode: " << info.unicode();
	return ss.str();
}

void OutputFactoryInfo(FactoryInfo const &info)
{
	hwm::wdout << "--- Output Factory Info ---" << std::endl;
	hwm::wdout << FactoryInfoToString(info) << std::endl;
}

Vst3PluginFactory::Impl::Impl(String module_path)
{
    Module mod(module_path.c_str());
	if(!mod) {
		throw std::runtime_error("cannot load library");
	}

    auto init_dll = reinterpret_cast<SetupProc>(mod.get_proc_address("InitDll"));
	if(init_dll) {
		init_dll();
	}

	//! GetPluginFactoryという名前でエクスポートされている、
	//! Factory取得用の関数を探す。
	GetFactoryProc get_factory = (GetFactoryProc)mod.get_proc_address("GetPluginFactory");
	if(!get_factory) {
		throw std::runtime_error("not a vst3 module");
	}

	auto factory = to_unique(get_factory());
	if(!factory) {
		throw std::runtime_error("Failed to get factory function");
	}
	
	PFactoryInfo loaded_factory_info;
	factory->getFactoryInfo(&loaded_factory_info);

	std::vector<ClassInfo> class_info_list;

	for(int i = 0; i < factory->countClasses(); ++i) {
		auto maybe_factory3 = queryInterface<IPluginFactory3>(factory);
		if(maybe_factory3.is_right()) {
			PClassInfoW info;
			maybe_factory3.right()->getClassInfoUnicode(i, &info);
			class_info_list.emplace_back(info);
		} else {
			auto maybe_factory2 = queryInterface<IPluginFactory2>(factory);
			if(maybe_factory2.is_right()) {
				PClassInfo2 info;
				maybe_factory2.right()->getClassInfo2(i, &info);
				class_info_list.emplace_back(info);
			} else {
				PClassInfo info;
				factory->getClassInfo(i, &info);
				class_info_list.emplace_back(info);
			}
		}
	}

	module_ = std::move(mod);
	factory_ = std::move(factory);
	factory_info_ = FactoryInfo(loaded_factory_info);
	class_info_list_ = std::move(class_info_list);

	OutputFactoryInfo(factory_info_);
	OutputClassInfoList(class_info_list_);
}

Vst3PluginFactory::Impl::~Impl()
{
	factory_.reset();

	if(module_) {
		auto exit_dll = (SetupProc)module_.get_proc_address("ExitDll");
		if(exit_dll) {
			exit_dll();
		}
	}
}

Vst3PluginFactory::Vst3PluginFactory(String module_path)
{
	pimpl_ = std::unique_ptr<Impl>(new Impl(module_path));
}

Vst3PluginFactory::~Vst3PluginFactory()
{}

FactoryInfo const &
		Vst3PluginFactory::GetFactoryInfo() const
{
	return pimpl_->GetFactoryInfo();
}

//! PClassInfo::category が kVstAudioEffectClass のもののみ
size_t	Vst3PluginFactory::GetComponentCount() const
{
	return pimpl_->GetComponentCount();
}

ClassInfo const &
		Vst3PluginFactory::GetComponentInfo(size_t index)
{
	return pimpl_->GetComponent(index);
}

std::unique_ptr<Vst3Plugin>
		Vst3PluginFactory::CreateByIndex(size_t index, host_context_type host_context)
{
	return CreatePlugin(pimpl_->GetFactory(), GetComponentInfo(index), std::move(host_context));
}

std::unique_ptr<Vst3Plugin>
		Vst3PluginFactory::CreateByID(Steinberg::int8 const * component_id, host_context_type host_context)
{
	for(size_t i = 0; i < GetComponentCount(); ++i) {
		bool const is_equal = std::equal<Steinberg::int8 const *, Steinberg::int8 const *>(
			GetComponentInfo(i).cid(), GetComponentInfo(i).cid() + 16,
			component_id
			);

		if(is_equal) {
			return CreateByIndex(i, std::move(host_context));
		}
	}

	throw std::runtime_error("No specified id in this factory.");
}

} // ::hwm

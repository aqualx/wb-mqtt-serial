#include "ir_device_query_handler.h"
#include "ir_device_query_factory.h"
#include "ir_device_query.h"
#include "virtual_register.h"
#include "serial_device.h"

#include <cassert>

using namespace std;

namespace   // utility
{
    template <typename TCondition>
    void RecreateQueries(const PIRDeviceQuerySet & querySet, const TCondition & condition, TIRDeviceQueryFactory::EQueryGenerationPolicy policy, const char * actionName)
    {
        for (auto itQuery = querySet->Queries.begin(); itQuery != querySet->Queries.end();) {
            auto query = *itQuery;

            if (!condition(query)) {
                ++itQuery; continue;
            }

            cerr << "INFO: [IR device query handler] " << actionName << " on query " << query->Describe() << endl;

            std::list<TPSet<PMemoryBlock>> groupedRegisters;
            for (const auto & virtualRegister: query->VirtualRegisters) {
                groupedRegisters.push_back(virtualRegister->GetMemoryBlocks());
            }

            try {
                const auto & generatedQueries = TIRDeviceQueryFactory::GenerateQueries(move(groupedRegisters), query->Operation, policy);
                itQuery = querySet->Queries.erase(itQuery);
                querySet->Queries.insert(itQuery, generatedQueries.begin(), generatedQueries.end());

                cerr << "INFO: [IR device query handler] recreated query " << query->Describe() << " as " << PrintCollection(generatedQueries, [](ostream & s, const PIRDeviceQuery & query){
                    s << "\t" << query->Describe();
                }, true, "") << endl;

            } catch (const TSerialDeviceException & e) {
                query->SetAbleToSplit(false);

                cerr << "INFO: unable to recreate query " << query->Describe() << ": " << e.what() << endl;

                ++itQuery; continue;
            }
        }

        assert(!querySet->Queries.empty());
    }
}

void TIRDeviceQuerySetHandler::HandleQuerySetPostExecution(const PIRDeviceQuerySet & querySet)
{
    DisableHolesIfNeeded(querySet);
    SplitByRegisterIfNeeded(querySet);
    DisableRegistersIfNeeded(querySet);
    ResetQueriesStatuses(querySet);
    InvalidateReadValues(querySet);
}

void TIRDeviceQuerySetHandler::DisableHolesIfNeeded(const PIRDeviceQuerySet & querySet)
{
    static auto condition = [](const PIRDeviceQuery & query){
        return query->GetStatus() == EQueryStatus::DevicePermanentError
            && query->IsAbleToSplit()
            && query->HasHoles;
    };

    RecreateQueries(querySet, condition, TIRDeviceQueryFactory::NoHoles, "disable holes");
}

void TIRDeviceQuerySetHandler::SplitByRegisterIfNeeded(const PIRDeviceQuerySet & querySet)
{
    static auto condition = [](const PIRDeviceQuery & query){
        return query->GetStatus() == EQueryStatus::DevicePermanentError
            && query->IsAbleToSplit()
            && !query->HasHoles;
    };

    RecreateQueries(querySet, condition, TIRDeviceQueryFactory::AsIs, "split by register");
}

void TIRDeviceQuerySetHandler::DisableRegistersIfNeeded(const PIRDeviceQuerySet & querySet)
{
    static auto condition = [](const PIRDeviceQuery & query){
        return query->GetStatus() == EQueryStatus::DevicePermanentError
            && !query->IsAbleToSplit();
    };

    for (const auto & query: querySet->Queries) {
        if (!condition(query)) {
            continue;
        }

        cerr << "INFO: [IR device query handler] disable query " << query->Describe() << endl;

        query->SetEnabledWithRegisters(false);
    }
}

void TIRDeviceQuerySetHandler::ResetQueriesStatuses(const PIRDeviceQuerySet & querySet)
{
    for (const auto & query: querySet->Queries) {
        query->ResetStatus();
    }
}

void TIRDeviceQuerySetHandler::InvalidateReadValues(const PIRDeviceQuerySet & querySet)
{
    for (const auto & query: querySet->Queries) {
        query->InvalidateReadValues();
    }
}
/*
 This is part of the OTF-Profiler. Copyright by ZIH, TU Dresden 2016-2018.
 Authors: Maximillian Neumann, Denis Hünich, Jens Doleschal, Bill Williams
*/
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include "all_data.h"
#include "definitions.h"
#include "otf2/OTF2_GeneralDefinitions.h"
#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "access_pattern_detection.h"

using namespace rapidjson;
using std::cout;
using std::string;
using namespace definitions;

template <typename os_t>
class PlainWriter {
    os_t& m_stream;

   public:
    PlainWriter(os_t& output) : m_stream(output) {}
    template <typename T>
    void Key(T k) const {
        m_stream << k << ": ";
    }
    void Uint64(uint64_t u) const { m_stream << u << "\n"; }
    void Uint(uint32_t u) const { m_stream << u << "\n"; }
    void StartObject() const {}
    void EndObject() const {}
    void StartArray() const {}
    void EndArray() const {}
};

/* Arbitrary labeled Uint64-typed data associated with a statistic */
struct ProfileEntry {
	/* Internal storage of labeled integer-data */
    std::map<std::string, uint64_t> entries;

    void add_data(const std::string& key, uint64_t value) {
        if (value == 0 || value == (uint64_t)(-1))
            return;
        entries[key] += value;
    }

    template <typename Writer>
    void WriteProfile(Writer& w) const {
        w.StartObject();
        for (const auto& kv : entries) {
            w.Key(StringRef(kv.first.c_str()));
            w.Uint64(kv.second);
        }
        w.EndObject();
    }
};

using definitions::Definitions;
using definitions::IoHandle;
using AccessPatternTypeString = std::string;

namespace access_pattern {

	/**
	 * @brief Get most dominant access pattern from list of accessed offsets
	 */
	AccessPatternTypeString get_access_pattern_from_offsets(std::vector<uint64_t> offsets)
	{
		if(offsets.empty())
			// assume that this only happens in `O_APPEND`-mode
			return "contiguous";
		else {
			// 1) Check if strided (=equal distances btw offsets)
			std::vector<uint64_t> offset_diffs(offsets.size());
			std::adjacent_difference(offsets.begin(), offsets.end(), offset_diffs.begin());
			offset_diffs.erase(offset_diffs.begin()); // first elem is copied from original vec when using `adjacent_difference()`

			if (std::all_of(offset_diffs.begin(), offset_diffs.end(), [&offset_diffs](uint64_t x){ return x == offset_diffs[0]; }))
				return "strided";
			else
				return "random";
		}
	}
};

/**
 *	Statistics stored per file
 */
struct FileInfo {
    FileInfo() : parentfile(NULL) {}
	/* Construct FileInfo given file_ref-id (from Definitions) */
    FileInfo(const Definitions& defs, OTF2_IoHandleRef id) {
        parentfile           = NULL;
        const IoHandle* ioh = defs.iohandles.get(id);
        if (!ioh)
            return;
        const IoHandle* parent = defs.iohandles.get(ioh->parent);
        if (parent)
            parentfile = new FileInfo(defs, ioh->parent);
        filename = ioh->file_handle->file_name;
        paradigm.insert(defs.io_paradigms.get(ioh->io_paradigm)->name);
        modes = ioh->modes;


		std::cout << ioh->file_handle->file_name << std::endl;
		// some io-handles might not contain a location (TODO: check!)
		if (ioh->location.has_value()) {
			auto location = ioh->location.value();
			locations.insert(location);
		}
    }

	// TODO: Remove Operator Overloading, make function more explicit
    void operator+=(const FileInfo& rhs) {
        filename = rhs.filename; // TODO: the file names should be always equal, right?
        if (rhs.parentfile && !parentfile)
            parentfile = rhs.parentfile;
        std::copy(rhs.paradigm.begin(), rhs.paradigm.end(), std::inserter(paradigm, paradigm.begin()));
        std::copy(rhs.modes.begin(), rhs.modes.end(), std::inserter(modes, modes.begin()));

        std::copy(rhs.locations.begin(), rhs.locations.end(), std::inserter(locations, locations.begin()));
		// TODO: Size/Timing stats are collected elsewhere (`bytes_read`,`bytes_write`,`time_spent_in_ticks`)
	}

    std::string           filename;
	/* I/O Paradigms used to perform I/O on file */
    std::set<std::string> paradigm;
	/* Modes in which file was opened (read/write) */
	std::set<std::string> modes;
    FileInfo*             parentfile;
	/* Bytes read from this file */
	std::uint64_t		  bytes_read=0;
	/* Bytes written to this file */
	std::uint64_t		  bytes_write=0;
	/* Time spent reading/writing the file */
	std::uint64_t		  time_spent_in_ticks=0;

	// === Access Patterns (NEXT: TODO)
	/* @brief Store list of locations that accessed this file
	 * - implicitly determines whether file-access is exclusive ("E") or shared ("S") (just check whether .size==1?)
	 */
	std::unordered_set<OTF2_LocationRef> locations;

	/** How many ticks have been spent in each access pattern */
	std::map<AccessPattern, uint64_t> ticks_spent_per_access_pattern{};
	/** How many bytes have been worked upon per access pattern */
	std::map<AccessPattern, uint64_t> iosize_per_access_pattern{};


	// TODO: time spent for meta-ops

    template <typename Writer>
    void WriteFileInfo(Writer& w) const {
        w.StartObject();
        w.Key("FileName");
        w.String(filename.c_str());

        w.Key("IoParadigm");
        w.StartArray();
        for (auto pstr : paradigm) {
            w.String(pstr.c_str());
        }
        w.EndArray();
        w.Key("AccessModes");
        std::string merged_modes;
        for (auto modestr : modes) {
            merged_modes += modestr;
        }
        w.String(merged_modes.c_str());
        w.Key("ParentFile");
        if (parentfile && parentfile->filename != filename) {
            parentfile->WriteFileInfo(w);
        } else {
            w.Null();
        }

		w.Key("#Bytes read");
		w.Uint64(bytes_read);
		w.Key("#Bytes write");
		w.Uint64(bytes_write);
		w.Key("Ticks spent");
		w.Uint64(time_spent_in_ticks);

		w.Key("Nr accesses from different locations");
		w.Uint64(locations.size());

		// store which access patterns have been used for accessin the files (both how many locations accessed file by this access pattern & % of time of accessing file in this pattern
		w.Key("Ticks spent per Access Pattern");
		w.StartObject();
		for(auto& [access_pattern, ticks_spent] : ticks_spent_per_access_pattern) {
			w.Key(access_pattern_to_string(access_pattern));
			w.Uint64(ticks_spent);
		}
		w.EndObject();
		w.Key("I/O sizes per Access Pattern");
		w.StartObject();
		for(auto& [access_pattern, io_size] : iosize_per_access_pattern) {
			w.Key(access_pattern_to_string(access_pattern));
			w.Uint64(io_size);
		}
		w.EndObject();

        w.EndObject();
    }
};

/** @brief Stores stats per region for output
 * TODO: NEXT
 * */
struct RegionInfo {
    RegionInfo() {}
	/* Construct Region info given io_handle-id (from Definitions) */
    RegionInfo(const Definitions& defs, uint64_t id) {
        const IoHandle* self = defs.iohandles.get(id);
        if (!self)
            return;
        const IoHandle* parent = defs.iohandles.get(self->parent);
        region_name= self->file_handle->file_name;
        paradigm.insert(defs.io_paradigms.get(self->io_paradigm)->name);
        modes = self->modes;
    }

    void operator+=(const FileInfo& rhs) {
        region_name = rhs.filename;
        std::copy(rhs.paradigm.begin(), rhs.paradigm.end(), std::inserter(paradigm, paradigm.begin()));
        std::copy(rhs.modes.begin(), rhs.modes.end(), std::inserter(modes, modes.begin()));

		// TODO: Size/Timing stats are collected elsewhere (`bytes_read`,`bytes_write`,`time_spent_in_ticks`)
    }

	/* Region/Function name */
    std::string           region_name;
	std::string			  region_begin_src_line;
	std::string			  region_end_src_line;
	/* File in which this region/functions is defined */
	/* I/O Paradigms used to perform I/O on file */
    std::set<std::string> paradigm;
	/* Modes in which file was opened (read/write) */
	std::set<std::string> modes;
	// WIP:
	/* Bytes read from by this region */
	std::uint64_t		  bytes_read=0;
	/* Bytes written by this region */
	std::uint64_t		  bytes_write=0;
	/* Time spent reading/writing by this region */
	std::uint64_t		  time_spent_in_ticks=0;
	/* Top 5 other regions that called this region (by nr of calls) with `std::string` being the name of the calling region
	 * @note data is taken from @ref{AllData} (field `const_cast<std::map<const Region*, uint64_t>&>(my_map);`)
	 * */
	std::map<std::string, uint64_t> region_callees_top5; // TODO
	// TODO: time spent for meta-ops

    template <typename Writer>
    void WriteRegionInfo(Writer& w, const std::map<std::string, RegionInfo>* io_per_region) const {
		if(io_per_region == nullptr) {
			std::cerr << "Encountered io_per_region=nullptr" << std::endl;
			return;
		}

        w.StartObject();
        w.Key("RegionName");
        w.String(region_name.c_str());

        w.Key("IoParadigm");
        w.StartArray();
        for (auto pstr : paradigm) {
            w.String(pstr.c_str());
        }
        w.EndArray();
        w.Key("AccessModes");
        std::string merged_modes;
        for (auto modestr : modes) {
            merged_modes += modestr;
        }
        w.String(merged_modes.c_str());

		w.Key("#Bytes read");
		w.Uint64(bytes_read);
		w.Key("#Bytes write");
		w.Uint64(bytes_write);
		w.Key("Ticks spent");
		w.Uint64(time_spent_in_ticks);

		w.Key("Top 5 Callees");
        w.StartArray();
        for (auto pstr : region_callees_top5) {
			std::string parent_begin_src_line = "?", parent_end_src_line = "?";
			if(io_per_region->find(pstr.first) != io_per_region->end()) {
				// TODO: src-lines not working (yet)
				auto parent_region_info = io_per_region->find(pstr.first)->second;
				parent_begin_src_line = parent_region_info.region_begin_src_line;
				parent_end_src_line = parent_region_info.region_end_src_line;
			}

			w.StartObject();
			w.Key("Region name");
            w.String(pstr.first.c_str());
			w.Key("Source lines");
			w.String((parent_begin_src_line + "-" + parent_end_src_line).c_str());
			w.Key("Nr calls");
			w.Uint64(pstr.second);
			w.EndObject();
        }
        w.EndArray();

        w.EndObject();
    }
};

/**
 * Data structure for storing resulting profile to output
 */
struct WorkflowProfile {
    uint64_t                            job_id;
	/* Nr of Nodes used during execution of traced program */
    uint32_t                            node_count;
	/* Nr of Processes used during execution of traced program */
    uint32_t                            process_count;
	/* Nr of Threads used during execution of traced program */
    uint32_t                            thread_count;
	/* Ticks per second */
    uint64_t                            timer_resolution;
    std::map<std::string, uint64_t>     counters;
    std::map<std::string, ProfileEntry> functions_by_paradigm;
    std::map<std::string, ProfileEntry> messages_by_paradigm;
	/* Nr of collective operations executed per paradigm */
    std::map<std::string, ProfileEntry> collops_by_paradigm;
	/* Nr of I/O operations executed per I/O paradigm */
    std::map<std::string, ProfileEntry> io_ops_by_paradigm;
	/* Statistics per file */
    std::map<std::string, FileInfo>     file_data;
	/* Statistics per srcline in a region */
	std::map<std::string, RegionInfo>   io_per_region; // TODO !
	/* Time (in ticks) spent executing parallel regions */
    uint64_t                            parallel_region_time;
	/* Time (in ticks) spent executing serial regions */
    uint64_t                            serial_time;
    uint64_t                            num_functions;
    uint64_t                            num_invocations;
	/* Path to otf2 trace-file (for which profile is being generated) */
    std::string                         filename;
    uint64_t                            traceID;
    template <typename Writer>
    void WriteProfile(Writer& w) const;
    WorkflowProfile()
        : job_id(0),
          node_count(0),
          process_count(0),
          thread_count(0),
          parallel_region_time(0),
          serial_time(0),
          num_functions(0),
          num_invocations(0) {}
};

template <typename Map, typename Writer>
void WriteMapUnderKey(std::string key, const Map& m, Writer& w) {
    if (m.empty())
        return;
    w.Key(StringRef(key.c_str()));
    w.StartObject();
    for (const auto& kv : m) {
        w.Key(StringRef(kv.first.c_str()));
        kv.second.WriteProfile(w);
    }
    w.EndObject();
}

template <typename Writer>
void WorkflowProfile::WriteProfile(Writer& w) const {
    w.StartObject();
    w.Key("Trace");
    w.StartObject();
    w.Key("FileName");
    w.String(filename.c_str());
    w.Key("Id");
    w.Uint64(traceID);
    w.EndObject();
    w.Key("JobId");
    w.Uint64(job_id);
    w.Key("NodeCount");
    w.Uint(node_count);
    w.Key("ProcessCount");
    w.Uint(process_count);
    w.Key("ThreadCount");
    w.Uint(thread_count);
    w.Key("TimerResolution");
    w.Uint64(timer_resolution);
    w.Key("HardwareCounters");
    w.StartArray();
    for (const auto& c : counters) {
        w.Key(StringRef(c.first.c_str()));
        w.Uint64(c.second);
    }
    w.EndArray();
    WriteMapUnderKey("Functions", functions_by_paradigm, w);
    WriteMapUnderKey("Messages", messages_by_paradigm, w);
    WriteMapUnderKey("CollectiveOperations", collops_by_paradigm, w);
    WriteMapUnderKey("IOOperations", io_ops_by_paradigm, w);

    w.Key("Files");
    w.StartArray();
    for (auto f : file_data) {
        f.second.WriteFileInfo(w);
    }
    w.EndArray();

    w.Key("Regions");
    w.StartArray();
    for (auto f : io_per_region) {
        f.second.WriteRegionInfo(w, &io_per_region);
    }
    w.EndArray();

    // WriteMapUnderKey("Regions", io_per_region, w);

    w.Key("ParallelRegionTime");
    w.Uint64(parallel_region_time);
    w.Key("SerialRegionTime");
    w.Uint64(serial_time);
    w.Key("TotalFunctions");
    w.Uint64(num_functions);
    w.Key("TotalCalls");
    w.Uint64(num_invocations);
    w.EndObject();
}

bool CreateJSON(AllData& alldata) {
    cout << "Creating JSON profile" << std::endl;
    WorkflowProfile            profile;
    StringBuffer               b;
    PrettyWriter<StringBuffer> w(b);
    for (const auto& n : alldata.definitions.system_tree) {
        switch (n.data.class_id) {
            case definitions::SystemClass::LOCATION:
                profile.thread_count++;
                break;
            case definitions::SystemClass::LOCATION_GROUP:
                profile.process_count++;
                break;
            case definitions::SystemClass::NODE:
                profile.node_count++;
                break;
            default:
                break;
        }
    }
    static std::string bytestr("Bytes");
    static std::string timestr("Time");
    static std::string countstr("Count");
    profile.timer_resolution = alldata.metaData.timerResolution;

    for (const auto& call_node : alldata.call_path_tree) {
        const auto& r = alldata.definitions.regions.get(call_node.function_id);
        if (!r) {
            continue;
        }

        const auto& p        = alldata.definitions.paradigms.get(r->paradigm_id);
        std::string paradigm = "COMPUTE";
        if (p) {
            paradigm = p->name;
        }

        profile.num_functions++;
        uint64_t excl_time = 0;

        for (const auto& one_node_data : call_node.node_data) {
            profile.num_invocations += one_node_data.second.f_data.count;
            auto& entries_map = profile.functions_by_paradigm[paradigm].entries;
            entries_map[countstr] += one_node_data.second.f_data.count;
            if (excl_time < one_node_data.second.f_data.excl_time) {
                excl_time = one_node_data.second.f_data.excl_time;
            }
            auto&       message_entry = profile.messages_by_paradigm[paradigm];
            auto&       collop_entry  = profile.collops_by_paradigm[paradigm];
            const auto& message_data  = one_node_data.second.m_data;
            const auto& collop_data   = one_node_data.second.c_data;
            message_entry.add_data(bytestr, message_data.bytes_send);
            message_entry.add_data(bytestr, message_data.bytes_recv);
            message_entry.add_data(countstr, message_data.count_send);
            message_entry.add_data(countstr, message_data.count_recv);
            if (one_node_data.second.c_data.bytes_send)
                profile.collops_by_paradigm[paradigm].entries[bytestr] += one_node_data.second.c_data.bytes_send;
            if (one_node_data.second.c_data.bytes_recv)
                profile.collops_by_paradigm[paradigm].entries[bytestr] += one_node_data.second.c_data.bytes_recv;
            if (one_node_data.second.c_data.count_send)
                profile.collops_by_paradigm[paradigm].entries[countstr] += one_node_data.second.c_data.count_send;
            if (one_node_data.second.c_data.count_recv)
                profile.collops_by_paradigm[paradigm].entries[countstr] += one_node_data.second.c_data.count_recv;
            for (const auto& metric : one_node_data.second.metrics) {
                if (metric.second.type == MetricDataType::UINT64) {
                    auto m = alldata.definitions.metrics.get(metric.first);
                    if (m)
                        profile.counters[m->name] += (uint64_t)metric.second.data_excl;
                }
            }
        }
        if (call_node.node_data.size() == 1) {
            profile.serial_time += excl_time;
        } else {
            profile.parallel_region_time += excl_time;
        }
        profile.functions_by_paradigm[paradigm].entries[timestr] += excl_time;
    }
    static std::string meta_time     = "MetaOperationTime";
    static std::string transfer_time = "TransferOperationTime";

	/* 1) Store stats per paradigm */
	cout << "Nr io_data:" << alldata.io_data_per_paradigm.size() << std::endl;
    for (auto io_entry : alldata.io_data_per_paradigm) {
        std::string paradigm_name = alldata.definitions.io_paradigms.get(io_entry.first)->name;
		IoData io_data = io_entry.second;
        cout << "Summarizing io for " << paradigm_name << " with bytes=" << io_data.num_bytes << std::endl;
        profile.io_ops_by_paradigm[paradigm_name].entries[bytestr] += io_data.num_bytes;
        profile.io_ops_by_paradigm[paradigm_name].entries[countstr] += io_data.num_operations;
        profile.io_ops_by_paradigm[paradigm_name].entries[transfer_time] += io_data.transfer_time;
        profile.io_ops_by_paradigm[paradigm_name].entries[meta_time] += io_data.nontransfer_time;
    }

	/* 2) Store stats per file */
    for (auto& [file_name, file]: alldata.definitions.filehandles) {
		for (auto& ioh_id : file->io_handles) {
			auto ioh = alldata.definitions.iohandles.get(ioh_id);
			// auto file_name = ioh->file_handle->file_name;
			profile.file_data[file_name] += FileInfo(alldata.definitions, ioh_id);

			auto io_data = ioh->io_data_stats;
			uint64_t bytes_read = 0, bytes_write = 0;
			if(io_data.mode=="R") {
				bytes_read = io_data.num_bytes;
				bytes_write = 0;
			} else if(io_data.mode=="W") {
				bytes_read = 0;
				bytes_write = io_data.num_bytes;
			} else if (io_data.num_bytes > 0) {
				cout << "[WARNING] Invalid io-access-mode:" << io_data.mode << ", Nr bytes worked on are:" << io_data.num_bytes << std::endl;
			}
			// profile.file_data[file_name].filename = file_name;
			profile.file_data[file_name].bytes_write += bytes_write;
			profile.file_data[file_name].bytes_read += bytes_read;
			profile.file_data[file_name].time_spent_in_ticks += io_data.transfer_time;
			profile.file_data[file_name].time_spent_in_ticks += io_data.nontransfer_time; // TODO: output `nontransfer_time` separately as metadata-ops-time?

			// get local access pattern
			auto access_pattern = ioh->get_access_pattern();
			profile.file_data[file_name].ticks_spent_per_access_pattern[access_pattern]
				+= io_data.transfer_time;
			profile.file_data[file_name].iosize_per_access_pattern[access_pattern]
				+= bytes_read + bytes_write; // one of those is =0
		}
	}

	/* 3) Store stats per location */
	for(auto location_io_entry: alldata.io_data_per_location) {
		// TODO: NEXT
		auto io_data = location_io_entry.second;
		auto region = alldata.definitions.regions.get(io_data.region);
		auto region_name = region->name;
		auto begin_src_line = region->begin_source_line == OTF2_UNDEFINED_UINT32 ? "?" : std::to_string(region->begin_source_line);
		auto end_src_line = region->end_source_line == OTF2_UNDEFINED_UINT32 ? "?" : std::to_string(region->end_source_line);

		uint64_t bytes_read = 0, bytes_write = 0;
		if(io_data.mode=="R") {
			bytes_read = io_data.num_bytes;
			bytes_write = 0;
		} else if(io_data.mode=="W") {
			bytes_read = 0;
			bytes_write = io_data.num_bytes;
		} else if (io_data.num_bytes > 0) {
			cout << "[WARNING] Invalid io-access-mode:" << io_data.mode << ", Nr bytes worked on are:" << io_data.num_bytes << std::endl;
		}

		auto idx = region_name;
		profile.io_per_region[idx].region_name += region_name;
		profile.io_per_region[idx].bytes_write += bytes_write;
		profile.io_per_region[idx].bytes_read += bytes_read;
		profile.io_per_region[idx].time_spent_in_ticks += io_data.transfer_time;
		profile.io_per_region[idx].time_spent_in_ticks += io_data.nontransfer_time; // TODO: output `nontransfer_time` separately?
		profile.io_per_region[idx].region_begin_src_line = begin_src_line;
		profile.io_per_region[idx].region_end_src_line = end_src_line;

		// TODO: get 5 top-calling callees from `alldata`:
		auto all_callee_regions = alldata.parent_regions_by_callcount[io_data.region];
		std::vector<std::pair<OTF2_RegionRef, uint64_t>> all_callee_regions_sorted(all_callee_regions.begin(), all_callee_regions.end()); // map to vector of pairs to sort then
		std::sort(all_callee_regions_sorted.begin(), all_callee_regions_sorted.end(), [](const std::pair<OTF2_RegionRef, uint64_t>& a, const std::pair<OTF2_RegionRef, uint64_t>& b) {
			return a.second > b.second;  // Sort in descending order based on the value
		});

		// Step 3: Get the first 5 elements (top 5 largest values)
		for (int i = 0; i < 5 && i < all_callee_regions_sorted.size(); ++i) {
			auto parent_region_ref= all_callee_regions_sorted[i].first;
			auto parent_region_name =  alldata.definitions.regions.get(parent_region_ref)->name;
			auto call_count = all_callee_regions_sorted[i].second;

			profile.io_per_region[idx].region_callees_top5[parent_region_name] = call_count;
		}
	}

    profile.filename = alldata.params.input_file_name;
    profile.traceID  = alldata.traceID;
    profile.WriteProfile(w);
    string        fname = alldata.params.output_file_prefix + ".json";
    std::ofstream outfile(fname.c_str());
    outfile << b.GetString() << std::endl;
    return true;
}

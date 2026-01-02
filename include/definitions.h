/*
 This is part of the OTF-Profiler. Copyright by ZIH, TU Dresden 2016-2018.
 Authors: Maximillian Neumann, Denis Hünich, Jens Doleschal
*/

#pragma once
#include <algorithm>
#include <cstdint>
#include <optional>
#include "access_pattern_detection.h"
#include "otf2/OTF2_GeneralDefinitions.h"
#ifndef DEFINITIONS_H
#define DEFINITIONS_H

#include <cassert>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

#include "main_structs.h"

using namespace access_pattern_detection;

namespace definitions {

using paradigm_id_t = uint32_t;

/* @brief Stores data associated with each region as read from the Region-Definitions
 *
 * @note To get info about parent regions (=regions which called this region and how many times they did it) see @ref{AllData} (field `parent_regions_by_callcount`)
 */
struct Region {
    std::string   				name;
    paradigm_id_t 				paradigm_id;
    uint32_t      				begin_source_line;
	uint32_t      				end_source_line;
	std::string   				file_name;
};

/* OTF2 Attribute */
struct Attribute
{
	std::string name;
	std::string description;
	OTF2_Type   type;
};


/*OTF2 Metric */

struct Metric {
    std::string     name;
    std::string     description;
    MetricType      metricType;     // PAPI, etc.
    MetricMode      metricMode;     // accumulative, relative, etc.
    MetricDataType  type;           // OTF2_TYPE_INT64, OTF2_TYPE_UINT64, OTF2_TYPE_DOUBLE
    MetricBase      base;           // binary or decimal
    int64_t         exponent;       // Metric value scaled by factor base^exponent to get value in its base unit
    std::string     unit;           // "bytes", "operations", or "seconds"
    bool            allowed;        // ?
};

struct Metric_Class {
    uint8_t                     num_of_metrics;
    std::map<uint8_t, uint32_t> metric_member;
    MetricOccurrence            metric_occurrence;
    RecorderKind                recorder_kind;
};

struct Paradigm {
    std::string name;
    // TODO  paradigm class
};

struct Group {
    std::string           name;
    uint8_t               type;
    paradigm_id_t         paradigm_id;
    std::vector<uint64_t> members;
};

template <typename Id, typename TypeProperties>
class DefinitionType {
   public:
    using ContainerTypeProps_t = std::map<Id, TypeProperties>;
    using TypeProperties_t     = TypeProperties;

    DefinitionType() = default;

    DefinitionType(const ContainerTypeProps_t& props) : all_properties(props) {}

    void add(Id id, const TypeProperties_t& props) { all_properties[id] = props; }

    const TypeProperties_t* get(Id id) const {
        const auto props_it = all_properties.find(id);

        if (props_it == all_properties.end())
            // TODO Error handling
            return nullptr;

        return &(props_it->second);
    }

    const ContainerTypeProps_t& get_all() const { return all_properties; }

   private:
    ContainerTypeProps_t all_properties;
};

enum class SystemClass : uint8_t {
    OTHER,
    LOCATION,
    LOCATION_GROUP,
    NODE,
    BLADE,
    CAGE,
    CABINET,
    CABINET_ROW,
    MACHINE,
    UNKNOWN

};

class SystemIterator;

class SystemTree {
   public:
    struct SystemData {
        uint32_t    node_id;  // TODO ist traceid
        std::string name;
        SystemClass class_id;
        // test
        uint64_t location_id;
        uint32_t level;
    };

    struct SystemNode {
        using Children_t = std::map<uint32_t, std::shared_ptr<SystemNode>>;

        SystemNode* parent;
        Children_t  children;  // TODO muss über eigene ids gehen
        SystemData  data;

        SystemNode(SystemNode* _parent, const Children_t& _children, const SystemData& _data)
            : parent(_parent), children(_children), data(_data) {}
    };

   public:
    using SystemData_t = SystemData;
    using SystemNode_t = SystemNode;
    using iterator     = SystemIterator;

    SystemTree() = default;

    void insert_node(const SystemNode_t& node) {
        auto* parent = node.parent;

        if (parent != nullptr)
            insert_node(node.data.name, node.data.location_id, node.data.class_id, node.parent->data.location_id);
        else
            insert_node(node.data.name, node.data.location_id, node.data.class_id, static_cast<uint32_t>(-1));
    }
    // void insert_node(SystemTreeNode* aNode);
    const std::shared_ptr<SystemNode_t> insert_node(std::string name, uint64_t id, SystemClass class_id,
                                                    uint64_t parent_id) {
        SystemNode* parent = nullptr;
        if (class_id == SystemClass::LOCATION)
            parent = location_grps[parent_id];
        else if (parent_id != static_cast<uint32_t>(-1))
            parent = system_nodes[parent_id];

        auto new_node = std::make_shared<SystemNode_t>(SystemNode_t(parent, {}, {_size, name, class_id, id, 0}));

        if (parent != nullptr) {
            parent->children.insert(std::make_pair(_size, new_node));
            new_node->data.level = parent->data.level + 1;

            if (new_node->data.level < num_nodes_per_level.size())
                ++num_nodes_per_level[new_node->data.level];
            else
                num_nodes_per_level.push_back(1);
        } else {
            num_nodes_per_level.push_back(1);
            root = new_node;
        }

        ++_size;

        if (class_id == SystemClass::LOCATION) {
            locations.insert(std::make_pair(id, new_node.get()));
            return new_node;
        }

        if (class_id == SystemClass::LOCATION_GROUP)
            location_grps.push_back(new_node.get());
        else
            system_nodes.push_back(new_node.get());

        return new_node;
    }

    // insert node with predetermined IDs, necessary for jsonreader
    const std::shared_ptr<SystemNode_t> insert_node(std::string name,
                                                    uint32_t    node_id,
                                                    SystemClass class_id,
                                                    uint64_t    parent_id,
                                                    uint64_t    location_id,
                                                    uint64_t    parent_location_id
                                                    ) {

        SystemNode* parent = nullptr;

        if (class_id == SystemClass::LOCATION)
            parent = location_grps[parent_location_id];
        else if (parent_id != static_cast<uint32_t>(-1))
            parent = system_nodes[parent_id];

        auto new_node = std::make_shared<SystemNode_t>(SystemNode_t(parent, {}, {node_id, name, class_id, location_id, 0}));

        if (parent != nullptr) {
            parent->children.insert(std::make_pair(node_id, new_node));
            new_node->data.level = parent->data.level + 1;

            if (new_node->data.level < num_nodes_per_level.size())
                ++num_nodes_per_level[new_node->data.level];
            else
                num_nodes_per_level.push_back(1);
        } else {
            num_nodes_per_level.push_back(1);
            root = new_node;
        }

        ++_size;

        switch(class_id){
            case SystemClass::LOCATION_GROUP :
                location_grps.push_back(new_node.get());
                break;
            case SystemClass::LOCATION :
                locations[location_id] = new_node.get();
                break;
            default :
                system_nodes.push_back(new_node.get());
        }

        return new_node;

    }

    std::pair<uint8_t, std::unique_ptr<SystemTree>> summarize(const SystemTree& sys_tree, uint64_t limit) {
        // find the level where the count of nodes is bigger then the set limit
        bool   do_it = false;
        size_t level = 0;
        for (level = 0; level < num_nodes_per_level.size(); ++level) {
            if (num_nodes_per_level[level] > limit) {
                if (level > 1) {
                    if (num_nodes_per_level[level - 1] > 1) {
                        do_it = true;
                        break;
                    }
                }
                // TODO überspringt effektiv den fall wenn (level - 2) > 1 ist, etc.
                // summarize can't be done -> (level - 1) has < 2 elements
                return std::make_pair(2, nullptr);
            }
        }
        if (do_it)
            return std::make_pair(1, std::unique_ptr<SystemTree>(copy_reduced(sys_tree, level)));

        return std::make_pair(0, nullptr);
    }

    const std::shared_ptr<SystemNode_t> get_root() const { return root; }

    const size_t num_level() const { return num_nodes_per_level.size(); }

    const std::vector<uint32_t>& all_level() const { return num_nodes_per_level; }

    const uint32_t size() const { return _size; }

    SystemNode_t* location(size_t location_id) {
        auto it = locations.find(location_id);
        if (it != locations.end())
            return it->second;

        return nullptr;
    }

    iterator begin() const;

    iterator end() const;

   private:
    SystemTree* copy_reduced(const SystemTree& sys_tree, uint32_t level);

   private:
    std::shared_ptr<SystemNode_t>     root;
    std::vector<SystemNode_t*>        system_nodes{};
    std::vector<SystemNode_t*>        location_grps{};
    std::map<uint64_t, SystemNode_t*> locations{};
    uint32_t                          _size = 0;
    // test
    std::vector<uint32_t> num_nodes_per_level;
};

// TODO iterator traits
class SystemIterator {
   private:
    using SystemData_t = typename SystemTree::SystemData_t;
    using SystemNode_t = typename SystemTree::SystemNode_t;

   public:
    SystemIterator(const std::shared_ptr<SystemNode_t>& root) : current_ptr(root.get()), root_ptr(root) {
        assert(root.get() != nullptr);
    }

    SystemIterator(const std::shared_ptr<SystemNode_t>& root, SystemNode_t* node) : current_ptr(node), root_ptr(root) {
        assert(root.get() != nullptr);
    }

    SystemIterator() = default;

    SystemIterator(const SystemIterator& rhs_iter) = default;

    SystemIterator& operator=(const SystemIterator& rhs_iter) {
        current_ptr = rhs_iter.current_ptr;
        root_ptr    = rhs_iter.root_ptr;

        return *this;
    }

    SystemNode_t& operator*() {
        assert(current_ptr != nullptr);
        return *current_ptr;
    }

    SystemNode_t* operator->() {
        assert(current_ptr != nullptr);
        return current_ptr;
    }

    SystemIterator& operator++() {
        assert(current_ptr != nullptr);

        if (!current_ptr->children.empty())
            current_ptr = current_ptr->children.begin()->second.get();
        else {
            while (true) {
                if (current_ptr->parent != nullptr) {
                    auto child_it = current_ptr->parent->children.find(current_ptr->data.node_id);
                    ++child_it;

                    if (child_it == current_ptr->parent->children.end()) {
                        current_ptr = current_ptr->parent;
                        continue;
                    }

                    current_ptr = child_it->second.get();

                    return *this;
                }

                // got back to root -> end of iteration
                current_ptr = nullptr;
                break;
            }
        }

        return *this;
    }

    SystemIterator operator++(int) {
        SystemIterator it(*this);
        ++*this;

        return it;
    }

    bool operator==(const SystemIterator& rhs_it) {
        return ((current_ptr == rhs_it.current_ptr) && (root_ptr == rhs_it.root_ptr));
    }
    bool operator!=(const SystemIterator& rhs_it) {
        return ((current_ptr != rhs_it.current_ptr) || (root_ptr != rhs_it.root_ptr));
    }

   private:
    SystemNode_t*                 current_ptr;
    std::shared_ptr<SystemNode_t> root_ptr;
};

struct IoHandle; // Forward Declaration

/** @brief Represents a file, needed to know file-size (eg if offsets are given relative to fsize)
 *	@note Note the difference to @ref IoHandle which references an <u>open</u> file in a process/location
 *	@note A file is not actually a definition in OTF, but implicitly referenced behind an IoHandle
 *
 *	@ref FileInfo = struct holding statistics output in json
 */
struct File
{
    std::string file_name;
	/** Synchronize updates to `fsize` (which is affected by all writing locations
	 *  TODO: find different solution to make it work with MPI-parallel run
	 * */
	mutable std::unique_ptr<std::mutex> fsize_mutex;
	/** Track `fsize` to account for file-size-relative offset
	 * @note The file-size is relative to the file-size which the file had BEFORE the traced program
	 * was run (since we do not know how large the file actually was before and thus during the run)
	 */
	mutable uint64_t fsize = 0;

	/** All IoHandles that have been used to to perform I/O on this file
	 * - retrieve actual @ref IoHandle using @ref AllData definitions->iohandles
	 * @note ALl IoHandles are collected in @ref OTF2Reader::handle_def_io_handle
	 * */
	std::vector<OTF2_IoHandleRef> io_handles;

	File(std::string file_name): file_name(file_name), fsize_mutex(std::make_unique<std::mutex>()),
		fsize(0)
	{};

	/** Updates file size to new value by performing required synchronization
	 *
	 *	TODO: synchronize MPI-parallel runs
	 */
	inline void update_file_size(uint64_t new_fsize)
	{
		fsize_mutex->lock();
		fsize = new_fsize;
		fsize_mutex->unlock();
	}
};

using Fpos = uint64_t;
/** Represent a statistics associated with a file descriptor (=open file per location)
 *
 * @note For Stats per IoHandle (eg `transfer_time`=time actually spent doing I/O on this IoHandle)
 * see `&(alldata->io_data_per_io_handle[handle])`
 * TODO: associate `IoData` directly with IoHandle
 */
struct IoHandle {
	/** Statistics about I/O performed on this IoHandle
	 * 	- filled in @ref OTF2Reader::io_operation_complete_callback
	 * */
	mutable IoData io_data_stats;
	/** Location that created this IoHandle
	 * @note This is only set once the I/O-Handle is actually created (in the trace)
	 */
	mutable std::optional<OTF2_LocationRef> location = std::nullopt;
	/** File which is opened by IoHandle */
	std::shared_ptr<File> file_handle;
	/** the I/O paradigm used on this IoHandle (passed when creating the IoHandle) / opening the file */
    uint32_t    io_paradigm;
	/** TODO: what is this? Remove this? */
    OTF2_IoFileRef    	file;
	OTF2_IoHandleRef	self;
    OTF2_IoHandleRef    parent;
    // Defined by events, so we need to allow changes post-handle-definition
    // Ugly but that's the world we live in
    mutable std::set<std::string> modes;
	/** This is used to determine whether I/O-Op was a read or write (to append `numBytes` to r/w stats
	* accordingly) based on the `matchingId` passed to `io_operation_begin()`&`io_opertion_complete()`
	*/
	mutable std::map<uint64_t, std::string> used_io_mode;

	/* @brief List of I/O Accesses (timestamp of I/O completion,file position,I/O size))
	 * @note Set in @ref OTF2Reader::io_operation_begin_callback
	 *
	 * @note fpos is relative to the `fsize` the  corresponding File had BEFORE the traced program was run
	 * (since we can't know whether there was already sth written in that file from the trace)
	 * */
	mutable IOAccesses io_accesses;

	/** Track current `fpos` into file, determines position at which I/O is being performed
	 * @note `fpos` could be different from the actual fpos during the run (since the original fsize before the program
	 * executed is not given). But since our analysis works on relative offsets anyways, this shouldn't matter
	 * */
	mutable uint64_t fpos;

	IoHandle() = default;

	/** @note This Constructor is called during @ref OTF2Reader::handle_def_io_handle */
	IoHandle(OTF2_IoHandleRef self, std::shared_ptr<File> file_handle, uint32_t io_paradigm, uint64_t file, uint64_t parent)
		: self(self), file_handle(file_handle), io_paradigm(io_paradigm), file(file), parent(parent),
		  io_data_stats(IoData(self))
	{}

	/** Local Access Patterns are computed per IoHandle
	 * since the assumption is that local access patterns don't stretch btw opening&closing a file
	 */
	AnalysisResult get_access_pattern() const{
		return access_pattern_detection::detect_local_access_pattern(io_accesses);
	};
};

/**
 * @brief Data Structure for storing all definitions from OTF2-trace
 *
 * @note Have to be accessible globally !
 */
struct Definitions {
    DefinitionType<uint64_t, Region>        			regions;
    DefinitionType<uint64_t, Metric>        			metrics;
    DefinitionType<uint64_t, Metric_Class>  			metric_classes;
	DefinitionType<OTF2_AttributeRef, Attribute> 		attributes;
    DefinitionType<paradigm_id_t, Paradigm> 			paradigms;
    DefinitionType<paradigm_id_t, Paradigm> 			io_paradigms;
	/* TODO: What does the uint64_t here refer to?? Some internal indexing or OTf2-Refs?? */
	/// These are crated inside @ref OTF2Reader::handle_def_io_handle
    DefinitionType<OTF2_IoHandleRef, IoHandle>      	iohandles;
	/// Maps file-name (full-path) to corresponding FileHandle
	/// These are crated inside @ref OTF2Reader::handle_def_io_handle
	std::unordered_map<std::string, std::shared_ptr<File>>	filehandles;
    DefinitionType<uint64_t, Group>         			groups;
    SystemTree                              			system_tree{};
};

}  // namespace definitions

struct meta_data {
    std::map<uint64_t, uint64_t> communicators;

    std::map<uint64_t, std::string> processIdToName;

    std::map<uint64_t, std::string> metricIdToName;

    // std::map<uint64_t, metric_definition> metricIdToDef;

    // class_id, pair< number of the metric in class, metric_id >
    std::map<uint64_t, std::map<uint64_t, uint64_t>> metricClassToMetric;

    uint64_t timerResolution;
    uint32_t myRank;
    uint32_t numRanks;

#ifdef OTFPROFILER_MPI

    uint32_t packBufferSize;
    char*    packBuffer;

#endif

    meta_data(uint32_t my_rank = 0, uint32_t num_ranks = 1) : myRank(my_rank), numRanks(num_ranks), timerResolution(0) {
#ifdef OTFPROFILER_MPI

        packBufferSize = 0;
        packBuffer     = NULL;

#endif
    }

    ~meta_data() {
#ifdef OTFPROFILER_MPI

        freePackBuffer();

#endif
    }

#ifdef OTFPROFILER_MPI
    char* guaranteePackBuffer(uint32_t size) {
        if (packBufferSize < size) {
            packBufferSize = size;
            packBuffer     = (char*)realloc(packBuffer, packBufferSize * sizeof(char));

            assert(NULL != packBuffer);
        }

        return packBuffer;
    }

    void freePackBuffer() {
        if (packBuffer) {
            free(packBuffer);
            packBuffer = NULL;
        }
    }

    char* getPackBuffer() { return packBuffer; }

#endif /* OTFPROFILER_MPI */
};

#endif  // DEFINITIONS_H

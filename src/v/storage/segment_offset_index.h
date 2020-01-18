#pragma once
#include "model/fundamental.h"

#include <seastar/core/unaligned.hh>
#include <seastar/core/file.hh>

#include <memory>
#include <optional>
#include <vector>

namespace storage {

/**
 * file file format is: [ header ] [ payload ]
 * header  == segment_offset_index::header
 * payload == std::vector<pair<uint32_t,uint32_t>>;
 *
 * Assume an ntp("default", "test", 0);
 *     default/test/0/1-1-v1.log
 *
 * The name of this index _must_ be then:
 *     default/test/0/1-1-v1.log.offset_index
 */
class segment_offset_index {
public:
    struct [[gnu::packed]] header {
        /// xxhash of the payload only. does not include header in payload
        ss::unaligned<uint32_t> checksum{0};
        /// payload size
        ss::unaligned<uint32_t> size{0};
        /// reserved for future extensions, such as version and compression
        ss::unaligned<uint32_t> unused_flags{0};
    };
    // 32KB - a well known number as a sweet spot for fetching data from disk
    static constexpr size_t default_data_buffer_step = 4096 * 8;

    segment_offset_index(
      ss::sstring filename, ss::file, model::offset base, size_t step);

    void maybe_track(model::offset o, size_t pos, size_t data_size);
    std::optional<size_t> lower_bound(model::offset o);

    ss::future<> materialize_index();
    ss::future<> close();

private:
    ss::sstring _name;
    ss::file _out;
    model::offset _base;
    const size_t _step;
    size_t _acc{0};
    bool _needs_persistence{false};
    std::vector<std::pair<uint32_t, uint32_t>> _positions;
};

using segment_offset_index_ptr = std::unique_ptr<segment_offset_index>;

} // namespace storage
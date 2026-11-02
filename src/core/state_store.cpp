#include "state_store.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iterator>
#include <limits>
#include <utility>

namespace ew {
namespace {
constexpr uint8_t Magic[8] = {'E','W','R','S','T','A','T','E'};
constexpr size_t HeaderSize = 20;
constexpr size_t PlanSize = 2 + 7 * sizeof(float) + GridSide * GridSide * sizeof(float);
constexpr size_t ProjectSizeV1 = sizeof(uint64_t) + 1 + PlanSize;
constexpr size_t ProjectSizeV2 = sizeof(uint64_t) + 1 + sizeof(uint32_t) + PlanSize;
constexpr size_t ProjectSize = sizeof(uint64_t) + 1 + 2 * sizeof(uint32_t) + 2 * sizeof(float) + 1 + PlanSize;
constexpr size_t MaxFileSize = HeaderSize + 32 + PlanSize + MaxSavedProjects * ProjectSize;

void Put32(std::vector<uint8_t>& out, uint32_t v)
{
    for (unsigned i = 0; i < 4; ++i) out.push_back(static_cast<uint8_t>(v >> (i * 8)));
}
void Put64(std::vector<uint8_t>& out, uint64_t v)
{
    for (unsigned i = 0; i < 8; ++i) out.push_back(static_cast<uint8_t>(v >> (i * 8)));
}
void PutFloat(std::vector<uint8_t>& out, float v)
{
    uint32_t bits = 0;
    static_assert(sizeof(bits) == sizeof(v), "state format requires 32-bit floats");
    std::memcpy(&bits, &v, sizeof(bits)); Put32(out, bits);
}
uint32_t Crc32(const uint8_t* bytes, size_t size)
{
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < size; ++i) {
        crc ^= bytes[i];
        for (unsigned bit = 0; bit < 8; ++bit) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
    return ~crc;
}
struct Reader {
    const uint8_t* data; size_t size; size_t pos = 0;
    bool Take(void* out, size_t count) {
        if (pos > size || count > size - pos) return false;
        std::memcpy(out, data + pos, count); pos += count; return true;
    }
    bool U8(uint8_t* out) { return Take(out, 1); }
    bool U32(uint32_t* out) {
        uint8_t b[4]; if (!Take(b, sizeof(b))) return false;
        *out = static_cast<uint32_t>(b[0]) | (static_cast<uint32_t>(b[1]) << 8)
            | (static_cast<uint32_t>(b[2]) << 16) | (static_cast<uint32_t>(b[3]) << 24); return true;
    }
    bool U64(uint64_t* out) {
        uint8_t b[8]; if (!Take(b, sizeof(b))) return false;
        *out = 0; for (unsigned i = 0; i < 8; ++i) *out |= static_cast<uint64_t>(b[i]) << (i * 8); return true;
    }
    bool Float(float* out) {
        uint32_t bits = 0; if (!U32(&bits)) return false;
        std::memcpy(out, &bits, sizeof(bits)); return std::isfinite(*out);
    }
};
void PutPlan(std::vector<uint8_t>& out, const Plan& p)
{
    out.push_back(p.first ? 1 : 0); out.push_back(p.complete ? 1 : 0);
    PutFloat(out,p.a.x); PutFloat(out,p.a.y); PutFloat(out,p.a.z);
    PutFloat(out,p.b.x); PutFloat(out,p.b.y); PutFloat(out,p.b.z); PutFloat(out,p.target);
    for (float h : p.ground) PutFloat(out,h);
}
bool GetPlan(Reader& r, Plan* p)
{
    uint8_t first=0, complete=0;
    if (!p || !r.U8(&first) || !r.U8(&complete) || first>1 || complete>1) return false;
    p->first=first!=0; p->complete=complete!=0;
    if (!r.Float(&p->a.x) || !r.Float(&p->a.y) || !r.Float(&p->a.z)
        || !r.Float(&p->b.x) || !r.Float(&p->b.y) || !r.Float(&p->b.z) || !r.Float(&p->target)) return false;
    for (float& h : p->ground) if (!r.Float(&h)) return false;
    if (p->complete && (p->Width() <= 0 || p->Depth() <= 0)) return false;
    return true;
}
bool ValidPlan(const Plan& p)
{
    const float values[] = {p.a.x,p.a.y,p.a.z,p.b.x,p.b.y,p.b.z,p.target};
    for (float v : values) if (!std::isfinite(v)) return false;
    for (float h : p.ground) if (!std::isfinite(h)) return false;
    return !p.complete || (p.Width() > 0 && p.Depth() > 0);
}
}

bool EncodeState(const DurableState& state, std::vector<uint8_t>* bytes)
{
    if (!bytes || state.schema != StateSchemaVersion || state.projects.size() > MaxSavedProjects || !ValidPlan(state.draft)) return false;
    std::vector<uint8_t> payload;
    payload.reserve(32 + PlanSize + state.projects.size() * ProjectSize);
    payload.insert(payload.end(), state.terrainFingerprint.begin(), state.terrainFingerprint.end());
    PutPlan(payload,state.draft);
    for (size_t i=0; i<state.projects.size(); ++i) {
        const auto& p=state.projects[i];
        if (!p.id || static_cast<uint8_t>(p.status) > static_cast<uint8_t>(ProjectStatus::Cancelled) || !ValidPlan(p.plan) || !p.plan.complete
            || !std::isfinite(p.progress) || p.progress < 0 || p.progress > 1
            || !std::isfinite(p.workRemainderM3) || p.workRemainderM3 < 0) return false;
        for (size_t j=0; j<i; ++j) if (state.projects[j].id == p.id) return false;
        Put64(payload,p.id); payload.push_back(static_cast<uint8_t>(p.status)); Put32(payload,p.facilityId);Put32(payload,p.executionCursor);
        PutFloat(payload,p.progress); PutFloat(payload,p.workRemainderM3);
        payload.push_back(p.needsReview ? 1 : 0); PutPlan(payload,p.plan);
    }
    bytes->clear(); bytes->reserve(HeaderSize+payload.size());
    bytes->insert(bytes->end(),std::begin(Magic),std::end(Magic));
    Put32(*bytes,StateSchemaVersion); Put32(*bytes,static_cast<uint32_t>(payload.size())); Put32(*bytes,Crc32(payload.data(),payload.size()));
    bytes->insert(bytes->end(),payload.begin(),payload.end());
    return bytes->size() <= MaxFileSize;
}

bool DecodeState(const uint8_t* bytes, size_t size, DurableState* state)
{
    if (!bytes || !state || size < HeaderSize || size > MaxFileSize || std::memcmp(bytes,Magic,sizeof(Magic)) != 0) return false;
    Reader h{bytes,size,sizeof(Magic)};
    uint32_t version=0,payloadSize=0,crc=0;
    if (!h.U32(&version) || !h.U32(&payloadSize) || !h.U32(&crc) || (version < 1 || version > StateSchemaVersion)
        || payloadSize != size-HeaderSize || Crc32(bytes+HeaderSize,payloadSize) != crc) return false;
    const size_t projectSize=version==1?ProjectSizeV1:(version==2?ProjectSizeV2:ProjectSize);
    const size_t fixedPayload=32+PlanSize;
    if(payloadSize<fixedPayload || (payloadSize-fixedPayload)%projectSize!=0)return false;
    Reader r{bytes+HeaderSize,payloadSize};
    DurableState parsed; parsed.schema=version;
    if (!r.Take(parsed.terrainFingerprint.data(),parsed.terrainFingerprint.size()) || !GetPlan(r,&parsed.draft)) return false;
    while (r.pos < r.size) {
        if (parsed.projects.size() >= MaxSavedProjects) return false;
        SavedProject p; uint8_t status=0;
        if (!r.U64(&p.id) || !r.U8(&status) || !p.id || status>static_cast<uint8_t>(ProjectStatus::Cancelled)) return false;
        p.status=static_cast<ProjectStatus>(status);
        if(version>=2 && !r.U32(&p.facilityId))return false;
        if(version>=3) {
            uint8_t review=0;
            if(!r.U32(&p.executionCursor) || !r.Float(&p.progress) || !r.Float(&p.workRemainderM3) || !r.U8(&review) || review>1)return false;
            p.needsReview=review!=0;
        }
        if (!GetPlan(r,&p.plan) || !p.plan.complete) return false;
        for (const auto& prior : parsed.projects) if (prior.id==p.id) return false;
        parsed.projects.push_back(p);
    }
    parsed.schema=StateSchemaVersion;
    *state=std::move(parsed); return true;
}

bool FingerprintMatches(const DurableState& state, const std::array<uint8_t,32>& current) noexcept
{
    return state.terrainFingerprint == current;
}

} // namespace ew

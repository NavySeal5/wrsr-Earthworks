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
constexpr size_t PlanSize = 2 + 7 * sizeof(float) + 3 + MaxSitePoints * 3 * sizeof(float) + GridSide * GridSide * sizeof(float);
constexpr size_t MaxFileSize = HeaderSize + 32 + PlanSize + 4 + MaxSavedNameBytes + 64 + MaxSavedProjects * (16 + MaxSavedNameBytes + MaxProjectFacilities * (4 + 1 + 4 + 63 + 3 * sizeof(float)))
    + MaxSavedEarthworks * (33 + MaxSavedNameBytes + PlanSize);

void Put32(std::vector<uint8_t>& out, uint32_t v)
{
    for (unsigned i = 0; i < 4; ++i) out.push_back(static_cast<uint8_t>(v >> (i * 8)));
}
void Put64(std::vector<uint8_t>& out, uint64_t v)
{
    for (unsigned i = 0; i < 8; ++i) out.push_back(static_cast<uint8_t>(v >> (i * 8)));
}
void PutString(std::vector<uint8_t>& out,const std::string& value)
{
    Put32(out,static_cast<uint32_t>(value.size()));
    out.insert(out.end(),value.begin(),value.end());
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
    bool String(std::string* out,size_t maxLength=MaxSavedNameBytes) {
        uint32_t length=0;
        if(!out||!U32(&length)||length==0||length>maxLength||pos>size||length>size-pos)return false;
        out->assign(reinterpret_cast<const char*>(data+pos),length);pos+=length;
        for(unsigned char c:*out)if(c<0x20||c==0x7f)return false;
        return true;
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
    out.push_back(p.pointCount);
    out.push_back(static_cast<uint8_t>(p.safeAngles));out.push_back(0); // reserved for future safety modes
    for(unsigned i=0;i<p.pointCount;++i){PutFloat(out,p.points[i].x);PutFloat(out,p.points[i].y);PutFloat(out,p.points[i].z);}
    for (float h : p.ground) PutFloat(out,h);
}
bool GetLegacyPlan(Reader& r, Plan* p)
{
    uint8_t first=0, complete=0;
    if (!p || !r.U8(&first) || !r.U8(&complete) || first>1 || complete>1) return false;
    p->first=first!=0; p->complete=complete!=0;
    if (!r.Float(&p->a.x) || !r.Float(&p->a.y) || !r.Float(&p->a.z)
        || !r.Float(&p->b.x) || !r.Float(&p->b.y) || !r.Float(&p->b.z) || !r.Float(&p->target)) return false;
    for (float& h : p->ground) if (!r.Float(&h)) return false;
    if(p->complete) {
        p->pointCount=4;
        p->points[0]={std::min(p->a.x,p->b.x),p->target,std::min(p->a.z,p->b.z)};
        p->points[1]={std::max(p->a.x,p->b.x),p->target,std::min(p->a.z,p->b.z)};
        p->points[2]={std::max(p->a.x,p->b.x),p->target,std::max(p->a.z,p->b.z)};
        p->points[3]={std::min(p->a.x,p->b.x),p->target,std::max(p->a.z,p->b.z)};
        if (p->Width() <= 0 || p->Depth() <= 0) return false;
    } else if(p->first) {p->pointCount=1;p->points[0]=p->a;}
    p->RefreshBounds();
    return true;
}
bool GetPlan(Reader& r, Plan* p,uint32_t version)
{
    uint8_t first=0,complete=0,count=0;
    if(!p||!r.U8(&first)||!r.U8(&complete)||first>1||complete>1)return false;
    p->first=first!=0;p->complete=complete!=0;
    if(!r.Float(&p->a.x)||!r.Float(&p->a.y)||!r.Float(&p->a.z)||!r.Float(&p->b.x)||!r.Float(&p->b.y)
        ||!r.Float(&p->b.z)||!r.Float(&p->target)||!r.U8(&count)||count>MaxSitePoints)return false;
    if(version==9) {
        uint8_t safeRoad=0,safe=0;
        if(!r.U8(&safeRoad)||!r.U8(&safe)||safeRoad>1||safe>1)return false;
        // Preserve the stricter legacy behavior if both checks had been enabled.
        p->safeAngles=safeRoad?SafeAnglesMode::Roads:(safe?SafeAnglesMode::Vanilla:SafeAnglesMode::None);
    } else if(version>=10) {
        uint8_t mode=0,reserved=0;
        if(!r.U8(&mode)||!r.U8(&reserved)||mode>static_cast<uint8_t>(SafeAnglesMode::Vanilla)||reserved!=0)return false;
        p->safeAngles=static_cast<SafeAnglesMode>(mode);
    }
    p->pointCount=count;
    for(unsigned i=0;i<count;++i)if(!r.Float(&p->points[i].x)||!r.Float(&p->points[i].y)||!r.Float(&p->points[i].z))return false;
    for(float& h:p->ground)if(!r.Float(&h))return false;
    if(p->complete) {p->RefreshBounds();if(!ValidFootprint(*p))return false;}
    return !p->first||p->pointCount>0;
}
bool ValidPlan(const Plan& p)
{
    if(static_cast<uint8_t>(p.safeAngles)>static_cast<uint8_t>(SafeAnglesMode::Vanilla))return false;
    const float values[] = {p.a.x,p.a.y,p.a.z,p.b.x,p.b.y,p.b.z,p.target};
    for (float v : values) if (!std::isfinite(v)) return false;
    if(p.pointCount>MaxSitePoints)return false;
    for(unsigned i=0;i<p.pointCount;++i)if(!std::isfinite(p.points[i].x)||!std::isfinite(p.points[i].y)||!std::isfinite(p.points[i].z))return false;
    for (float h : p.ground) if (!std::isfinite(h)) return false;
    if(p.complete)return ValidFootprint(p);
    return !p.first||p.pointCount>0;
}
bool ValidName(const std::string& value)
{
    if(value.empty()||value.size()>MaxSavedNameBytes)return false;
    for(unsigned char c:value)if(c<0x20||c==0x7f)return false;
    return true;
}
bool ValidAssetName(const std::string& value)
{
    if(value.empty()||value.size()>63)return false;
    for(unsigned char c:value)if(c<0x20||c>0x7e)return false;
    return true;
}
}

bool EncodeState(const DurableState& state, std::vector<uint8_t>* bytes)
{
    if (!bytes || state.schema != StateSchemaVersion || state.projects.size() > MaxSavedProjects
        || state.earthworks.size() > MaxSavedEarthworks || !ValidPlan(state.draft) || !state.nextSiteNumber) return false;
    if(!ValidName(state.draftName))return false;
    std::vector<uint8_t> payload;
    payload.reserve(MaxFileSize-HeaderSize);
    payload.insert(payload.end(), state.terrainFingerprint.begin(), state.terrainFingerprint.end());
    PutPlan(payload,state.draft);PutString(payload,state.draftName);
    Put64(payload,state.selectedProjectId);Put32(payload,state.nextSiteNumber);
    Put32(payload,static_cast<uint32_t>(state.projects.size()));
    for (size_t i=0; i<state.projects.size(); ++i) {
        const auto& p=state.projects[i];
        if (!p.id || !ValidName(p.name) || p.facilityIds.size()>MaxProjectFacilities
            ||(!p.facilityAnchors.empty()&&p.facilityAnchors.size()!=p.facilityIds.size())) return false;
        for (size_t j=0; j<i; ++j) if (state.projects[j].id == p.id) return false;
        for(size_t j=0;j<p.facilityIds.size();++j) {
            if(!p.facilityIds[j])return false;
            for(size_t k=0;k<j;++k)if(p.facilityIds[k]==p.facilityIds[j])return false;
            for(size_t prior=0;prior<i;++prior)
                for(uint32_t priorFacility:state.projects[prior].facilityIds)if(priorFacility==p.facilityIds[j])return false;
        }
        Put64(payload,p.id);PutString(payload,p.name);Put32(payload,static_cast<uint32_t>(p.facilityIds.size()));
        for(size_t j=0;j<p.facilityIds.size();++j) {
            Put32(payload,p.facilityIds[j]);
            const bool hasAnchor=!p.facilityAnchors.empty()&&p.facilityAnchors[j].valid;
            payload.push_back(hasAnchor?1:0);
            if(hasAnchor) {
                const auto& anchor=p.facilityAnchors[j];
                if(!ValidAssetName(anchor.assetName)||!std::isfinite(anchor.position.x)||!std::isfinite(anchor.position.y)||!std::isfinite(anchor.position.z))return false;
                PutString(payload,anchor.assetName);
                PutFloat(payload,anchor.position.x);PutFloat(payload,anchor.position.y);PutFloat(payload,anchor.position.z);
            }
        }
    }
    auto selected=std::find_if(state.projects.begin(),state.projects.end(),[&](const SavedProject& p){return p.id==state.selectedProjectId;});
    if((state.projects.empty()&&state.selectedProjectId!=0)||(!state.projects.empty()&&selected==state.projects.end()))return false;
    Put32(payload,static_cast<uint32_t>(state.earthworks.size()));
    for(size_t i=0;i<state.earthworks.size();++i) {
        const auto& e=state.earthworks[i];
        if(!e.id||!ValidName(e.name)||
            std::find_if(state.projects.begin(),state.projects.end(),[&](const SavedProject& p){return p.id==e.projectId;})==state.projects.end()
            ||static_cast<uint8_t>(e.status)>static_cast<uint8_t>(ProjectStatus::Waiting)||!ValidPlan(e.plan)||!e.plan.complete
            ||!std::isfinite(e.progress)||e.progress<0||e.progress>1||!std::isfinite(e.totalWorkM3)||e.totalWorkM3<0
            ||!std::isfinite(e.workRemainderM3)||e.workRemainderM3<0)return false;
        for(size_t j=0;j<i;++j)if(state.earthworks[j].id==e.id)return false;
        Put64(payload,e.id);Put64(payload,e.projectId);PutString(payload,e.name);payload.push_back(static_cast<uint8_t>(e.status));
        Put32(payload,e.executionCursor);PutFloat(payload,e.progress);PutFloat(payload,e.totalWorkM3);PutFloat(payload,e.workRemainderM3);
        payload.push_back(e.needsReview?1:0);payload.push_back(e.visible?1:0);PutPlan(payload,e.plan);
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
    if (!h.U32(&version) || !h.U32(&payloadSize) || !h.U32(&crc) || (version != 5 && version != 6 && version != 7 && version != 8 && version != 9 && version != 10 && version != 11 && version != 12 && version != StateSchemaVersion)
        || payloadSize != size-HeaderSize || Crc32(bytes+HeaderSize,payloadSize) != crc) return false;
    Reader r{bytes+HeaderSize,payloadSize};
    DurableState parsed; parsed.schema=version;
    auto readPlan=[&](Plan* plan){return version>=7?GetPlan(r,plan,version):GetLegacyPlan(r,plan);};
    if (!r.Take(parsed.terrainFingerprint.data(),parsed.terrainFingerprint.size()) || !readPlan(&parsed.draft)) return false;
    if(version>=7&&!r.String(&parsed.draftName))return false;
    if(!r.U64(&parsed.selectedProjectId)||!r.U32(&parsed.nextSiteNumber)||!parsed.nextSiteNumber)return false;
    if(version==5) {
        char defaultName[MaxSavedNameBytes+1]{};sprintf_s(defaultName,"Site-%u",parsed.nextSiteNumber);
        parsed.draftName=defaultName;
    }
    uint32_t count=0;if(!r.U32(&count)||count>MaxSavedProjects)return false;
    for(uint32_t i=0;i<count;++i) {
        SavedProject p;uint32_t facilities=0;
        if(!r.U64(&p.id)||!p.id||!r.String(&p.name)||!r.U32(&facilities)||facilities>MaxProjectFacilities)return false;
        for(const auto& prior:parsed.projects)if(prior.id==p.id)return false;
        for(uint32_t j=0;j<facilities;++j) {
            uint32_t id=0;if(!r.U32(&id)||!id)return false;
            for(uint32_t prior:p.facilityIds)if(prior==id)return false;
            for(const auto& priorProject:parsed.projects)for(uint32_t prior:priorProject.facilityIds)if(prior==id)return false;
            p.facilityIds.push_back(id);
            SavedProject::FacilityAnchor anchor{};
            if(version>=11) {
                uint8_t valid=0;
                if(!r.U8(&valid)||valid>1)return false;
                if(valid) {
                    if(!r.String(&anchor.assetName,63)||!ValidAssetName(anchor.assetName)
                        ||!r.Float(&anchor.position.x)||!r.Float(&anchor.position.y)||!r.Float(&anchor.position.z))return false;
                    anchor.valid=true;
                }
            }
            p.facilityAnchors.push_back(std::move(anchor));
        }
        parsed.projects.push_back(std::move(p));
    }
    if((parsed.projects.empty()&&parsed.selectedProjectId!=0)||(!parsed.projects.empty()
        &&std::find_if(parsed.projects.begin(),parsed.projects.end(),[&](const SavedProject& p){return p.id==parsed.selectedProjectId;})==parsed.projects.end()))return false;
    if(!r.U32(&count)||count>MaxSavedEarthworks)return false;
    for(uint32_t i=0;i<count;++i) {
        SavedEarthwork e;uint8_t status=0,review=0,visible=1;
        const uint8_t maximumStatus=version<=11?6:static_cast<uint8_t>(ProjectStatus::Waiting);
        if(!r.U64(&e.id)||!e.id||!r.U64(&e.projectId)||!r.String(&e.name)||!r.U8(&status)
            ||status>maximumStatus||!r.U32(&e.executionCursor)||!r.Float(&e.progress)
            ||e.progress<0||e.progress>1)return false;
        if(version>=8&&(!r.Float(&e.totalWorkM3)||!std::isfinite(e.totalWorkM3)||e.totalWorkM3<0))return false;
        if(!r.Float(&e.workRemainderM3)||e.workRemainderM3<0||!r.U8(&review)||review>1)return false;
        if(version>=13&&(!r.U8(&visible)||visible>1))return false;
        if(!readPlan(&e.plan)||!e.plan.complete)return false;
        // Schemas through 11 used status 5 for Cancelled and 6 for Waiting.
        // Normalize Cancelled Sites to Paused and remove the obsolete state.
        if(version<=11) e.status=status==5?ProjectStatus::Paused
            :status==6?ProjectStatus::Waiting:static_cast<ProjectStatus>(status);
        else e.status=static_cast<ProjectStatus>(status);
        e.needsReview=review!=0;
        e.visible=visible!=0;
        if(std::find_if(parsed.projects.begin(),parsed.projects.end(),[&](const SavedProject& p){return p.id==e.projectId;})==parsed.projects.end())return false;
        for(const auto& prior:parsed.earthworks)if(prior.id==e.id)return false;
        parsed.earthworks.push_back(std::move(e));
    }
    if(r.pos!=r.size)return false;
    parsed.schema=StateSchemaVersion;
    *state=std::move(parsed); return true;
}

bool FingerprintMatches(const DurableState& state, const std::array<uint8_t,32>& current) noexcept
{
    return state.terrainFingerprint == current;
}

} // namespace ew

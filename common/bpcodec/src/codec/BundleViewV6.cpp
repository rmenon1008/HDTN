#include "codec/BundleViewV6.h"
#include <iostream>
#include <boost/next_prior.hpp>
#include "Uri.h"

void BundleViewV6::Bpv6PrimaryBlockView::SetManuallyModified() {
    dirty = true;
}
void BundleViewV6::Bpv6CanonicalBlockView::SetManuallyModified() {
    dirty = true;
}
void BundleViewV6::Bpv6CanonicalBlockView::SetBlockProcessingControlFlagAndDirtyIfNecessary(const uint64_t flag) {
    if ((header.flags <= 127) && (flag <= 127) && (actualSerializedHeaderAndBodyPtr.size() >= 2)) {
        header.flags |= flag;
        uint8_t * const data = static_cast<uint8_t*>(const_cast<void*>(actualSerializedHeaderAndBodyPtr.data()));
        data[1] = static_cast<uint8_t>(header.flags);
    }
    else {
        header.flags |= flag;
        dirty = true;
    }
}
void BundleViewV6::Bpv6CanonicalBlockView::ClearBlockProcessingControlFlagAndDirtyIfNecessary(const uint64_t flag) {
    if ((header.flags <= 127) && (flag <= 127) && (actualSerializedHeaderAndBodyPtr.size() >= 2)) {
        header.flags &= (~flag);
        uint8_t * const data = static_cast<uint8_t*>(const_cast<void*>(actualSerializedHeaderAndBodyPtr.data()));
        data[1] = static_cast<uint8_t>(header.flags);
    }
    else {
        header.flags &= (~flag);
        dirty = true;
    }
}
bool BundleViewV6::Bpv6CanonicalBlockView::HasBlockProcessingControlFlagSet(const uint64_t flag) const {
    return ((header.flags & flag) != 0);
}

BundleViewV6::BundleViewV6() {}
BundleViewV6::~BundleViewV6() {}

bool BundleViewV6::Load() {
    
    std::size_t offset = cbhe_bpv6_primary_block_decode(&m_primaryBlockView.header, (const char*)m_renderedBundle.data(), 0, m_renderedBundle.size());
    if (offset == 0) {
        return false;//Malformed bundle received
    }
    if (offset >= m_renderedBundle.size()) {
        return false;
    }
    if (m_primaryBlockView.header.flags & (BPV6_BUNDLEFLAG_FRAGMENT | BPV6_BUNDLEFLAG_ADMIN_RECORD)) {
        return false;
    }
    m_primaryBlockView.actualSerializedPrimaryBlockPtr = boost::asio::buffer(m_renderedBundle.data(), offset);
    m_primaryBlockView.dirty = false;

    while (true) {
        m_listCanonicalBlockView.emplace_back();
        Bpv6CanonicalBlockView & cbv = m_listCanonicalBlockView.back();
        cbv.dirty = false;
        cbv.markedForDeletion = false;
        bpv6_canonical_block & canonical = cbv.header;
        const uint32_t canonicalBlockHeaderSize = bpv6_canonical_block_decode(&canonical, (const char*)m_renderedBundle.data(), offset, m_renderedBundle.size());
        if (canonicalBlockHeaderSize == 0) {
            return false;
        }
        //m_mapCanonicalBlockTypeToPreexistingCanonicalBlockViews.insert(std::pair<uint8_t, Bpv6CanonicalBlockView*>(canonical.type, &cbv));
        const std::size_t totalCanonicalBlockSize = canonicalBlockHeaderSize + canonical.length;
        cbv.actualSerializedHeaderAndBodyPtr = boost::asio::buffer(((const uint8_t*)m_renderedBundle.data()) + offset, totalCanonicalBlockSize);
        cbv.actualSerializedBodyPtr = boost::asio::buffer(((const uint8_t*)m_renderedBundle.data()) + offset + canonicalBlockHeaderSize, canonical.length);
        offset += totalCanonicalBlockSize;
        if (canonical.flags & BPV6_BLOCKFLAG_LAST_BLOCK) {
            return (offset == m_renderedBundle.size());
        }
        else if (offset >= m_renderedBundle.size()) {
            return false;
        }
    }

}
bool BundleViewV6::Render(const std::size_t maxBundleSizeBytes) {
    //first render to the back buffer, copying over non-dirty blocks from the m_renderedBundle which may be the front buffer or other memory from a load operation
    m_backBuffer.resize(maxBundleSizeBytes);
    uint8_t * buffer = &m_backBuffer[0];
    uint8_t * const bufferBegin = buffer;
    if (m_primaryBlockView.dirty) {
        //std::cout << "pd\n";
        const uint64_t retVal = cbhe_bpv6_primary_block_encode(&m_primaryBlockView.header, (char *)buffer, 0, maxBundleSizeBytes);
        if (retVal == 0) {
            return false;
        }
        m_primaryBlockView.actualSerializedPrimaryBlockPtr = boost::asio::buffer(buffer, retVal);
        buffer += retVal;
        m_primaryBlockView.dirty = false;
    }
    else {
        //std::cout << "pnd\n";
        const std::size_t size = m_primaryBlockView.actualSerializedPrimaryBlockPtr.size();
        memcpy(buffer, m_primaryBlockView.actualSerializedPrimaryBlockPtr.data(), size);
        m_primaryBlockView.actualSerializedPrimaryBlockPtr = boost::asio::buffer(buffer, size);
        buffer += size;
    }
    if (m_primaryBlockView.header.flags & (BPV6_BUNDLEFLAG_FRAGMENT | BPV6_BUNDLEFLAG_ADMIN_RECORD)) {
        return false;
    }
    
    m_listCanonicalBlockView.remove_if([](const Bpv6CanonicalBlockView & v) { return v.markedForDeletion; }); //makes easier last block detection

    for (std::list<Bpv6CanonicalBlockView>::iterator it = m_listCanonicalBlockView.begin(); it != m_listCanonicalBlockView.end(); ++it) {
        const bool isLastBlock = (boost::next(it) == m_listCanonicalBlockView.end());
        if (isLastBlock) {
            //std::cout << "lb\n";
            it->SetBlockProcessingControlFlagAndDirtyIfNecessary(BPV6_BLOCKFLAG_LAST_BLOCK);
        }
        else {
            //std::cout << "nlb\n";
            it->ClearBlockProcessingControlFlagAndDirtyIfNecessary(BPV6_BLOCKFLAG_LAST_BLOCK);
        }
        if (it->dirty) {
            if (it->temporarySerialization.size() == 0) { //only the canonical block header needs recreated
                //std::cout << "cd_partial\n";
                const uint64_t sizeHeader = bpv6_canonical_block_encode(&it->header, (char *)buffer, 0, 0);
                if (sizeHeader <= 2) {
                    return false;
                }
                const std::size_t sizeBody = it->header.length;
                const std::size_t sizeHeaderAndBody = sizeHeader + sizeBody;
                it->actualSerializedHeaderAndBodyPtr = boost::asio::buffer(buffer, sizeHeaderAndBody);
                buffer += sizeHeader;
                memcpy(buffer, it->actualSerializedBodyPtr.data(), sizeBody);
                it->actualSerializedBodyPtr = boost::asio::buffer(buffer, sizeBody);
                buffer += sizeBody;
            }
            else { //copy over the external full block serialization
                //std::cout << "cd_full\n";
                const std::size_t sizeHeaderAndBody = it->temporarySerialization.size();
                if (sizeHeaderAndBody <= 2) {
                    return false;
                }
                const std::size_t sizeBody = it->header.length;
                const std::size_t sizeHeader = sizeHeaderAndBody - sizeBody;
                memcpy(buffer, it->temporarySerialization.data(), sizeHeaderAndBody);
                it->temporarySerialization = std::vector<uint8_t>();
                it->actualSerializedHeaderAndBodyPtr = boost::asio::buffer(buffer, sizeHeaderAndBody);
                it->actualSerializedBodyPtr = boost::asio::buffer(buffer + sizeHeader, sizeBody);
                buffer += sizeHeaderAndBody;
            }
            it->dirty = false;
        }
        else {
            //std::cout << "cnd\n";
            const std::size_t sizeHeaderAndBody = it->actualSerializedHeaderAndBodyPtr.size();
            const std::size_t sizeBody = it->actualSerializedBodyPtr.size();
            const std::size_t sizeHeader = sizeHeaderAndBody - sizeBody;
            if (sizeBody != it->header.length) {
                std::cout << sizeBody << " " << it->header.length << std::endl;
                return false;
            }
            memcpy(buffer, it->actualSerializedHeaderAndBodyPtr.data(), sizeHeaderAndBody);
                
            it->actualSerializedHeaderAndBodyPtr = boost::asio::buffer(buffer, sizeHeaderAndBody);
            it->actualSerializedBodyPtr = boost::asio::buffer(buffer + sizeHeader, sizeBody);
            buffer += sizeHeaderAndBody;
        }
    }
    m_backBuffer.resize(buffer - bufferBegin);
    m_frontBuffer.swap(m_backBuffer); //m_frontBuffer is now the rendered bundle
    m_renderedBundle = boost::asio::buffer(m_frontBuffer);
    return true;
}

void BundleViewV6::AppendPreserializedCanonicalBlock(const bpv6_canonical_block & header, std::vector<uint8_t> & serialized) {
    m_listCanonicalBlockView.emplace_back();
    Bpv6CanonicalBlockView & cbv = m_listCanonicalBlockView.back();
    cbv.dirty = true;
    cbv.markedForDeletion = false;
    cbv.header = header;
    cbv.temporarySerialization = std::move(serialized);
    cbv.actualSerializedHeaderAndBodyPtr = boost::asio::buffer(cbv.temporarySerialization);
    const std::size_t sizeHeaderAndBody = cbv.temporarySerialization.size();
    const std::size_t sizeBody = cbv.header.length;
    const std::size_t sizeHeader = sizeHeaderAndBody - sizeBody;
    cbv.actualSerializedBodyPtr = boost::asio::buffer(cbv.temporarySerialization.data() + sizeHeader, sizeBody);
}
std::size_t BundleViewV6::GetCanonicalBlockCountByType(const uint8_t canonicalBlockTypeCode) const {
    std::size_t count = 0;
    for (std::list<Bpv6CanonicalBlockView>::const_iterator it = m_listCanonicalBlockView.cbegin(); it != m_listCanonicalBlockView.cend(); ++it) {
        count += (it->header.type == canonicalBlockTypeCode);
    }
    return count;
}
std::size_t BundleViewV6::GetNumCanonicalBlocks() const {
    return m_listCanonicalBlockView.size();
}
void BundleViewV6::GetCanonicalBlocksByType(const uint8_t canonicalBlockTypeCode, std::vector<Bpv6CanonicalBlockView*> & blocks) {
    blocks.clear();
    for (std::list<Bpv6CanonicalBlockView>::iterator it = m_listCanonicalBlockView.begin(); it != m_listCanonicalBlockView.end(); ++it) {
        if (it->header.type == canonicalBlockTypeCode) {
            blocks.push_back(&(*it));
        }
    }
}
std::size_t BundleViewV6::DeleteAllCanonicalBlocksByType(const uint8_t canonicalBlockTypeCode) {
    std::size_t count = 0;
    for (std::list<Bpv6CanonicalBlockView>::iterator it = m_listCanonicalBlockView.begin(); it != m_listCanonicalBlockView.end();) {
        if (it->header.type == canonicalBlockTypeCode) {
            it = m_listCanonicalBlockView.erase(it);
            ++count;
        }
    }
    return count;
}
bool BundleViewV6::LoadBundle(uint8_t * bundleData, const std::size_t size) {
    Reset();
    m_renderedBundle = boost::asio::buffer(bundleData, size);
    return Load();
}
bool BundleViewV6::SwapInAndLoadBundle(std::vector<uint8_t> & bundleData) {
    Reset();
    m_frontBuffer.swap(bundleData);
    m_renderedBundle = boost::asio::buffer(m_frontBuffer);
    return Load();
}
bool BundleViewV6::CopyAndLoadBundle(const uint8_t * bundleData, const std::size_t size) {
    Reset();
    m_frontBuffer.assign(bundleData, bundleData + size);
    m_renderedBundle = boost::asio::buffer(m_frontBuffer);
    return Load();
}
bool BundleViewV6::IsValid() const {
    if (GetCanonicalBlockCountByType(BPV6_BLOCKTYPE_PAYLOAD) > 1) {
        return false;
    }
    return true;
}


void BundleViewV6::Reset() {
    //m_mapCanonicalBlockTypeToPreexistingCanonicalBlockViews.clear();
    m_listCanonicalBlockView.clear();


    m_renderedBundle = boost::asio::buffer((void*)NULL, 0);
    m_frontBuffer.clear();
    m_backBuffer.clear();
}


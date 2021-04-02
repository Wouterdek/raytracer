#pragma once

template<class Batch, typename MakeBatchFunc, class... MakeBatchFuncArgs>
size_t createBatches(std::vector<Batch>& batches, size_t totalCount, size_t batchSize, MakeBatchFunc makeBatch, MakeBatchFuncArgs&&... args)
{
    size_t batchCount = 0;
    for(size_t i = 0; i < totalCount; i += batchSize)
    {
        auto endIdx = std::min(i+batchSize, totalCount);
        batches.push_back(makeBatch(i, endIdx, std::forward<MakeBatchFuncArgs>(args)...));
        batchCount++;
    }
    auto remainingItems = totalCount % batchSize;
    if(remainingItems > 0)
    {
        auto startIdx = totalCount - remainingItems;
        auto endIdx = totalCount;
        batches.push_back(makeBatch(startIdx, endIdx, std::forward<MakeBatchFuncArgs>(args)...));
        batchCount++;
    }
    return batchCount;
}
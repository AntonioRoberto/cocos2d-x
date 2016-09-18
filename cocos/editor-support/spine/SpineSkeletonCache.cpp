#include "SpineSkeletonCache.h"

static auto error_log = &cocos2d::log;

namespace spine {
extern void(*onLoadTextureFailed)(const char* format, ...);

void SpineSkeletonCache::setErrorLogFunc(void(*errorfunc)(const char* pszFormat, ...))
{
	error_log = errorfunc;
	spine::onLoadTextureFailed = errorfunc;
}

SpineSkeletonCache* SpineSkeletonCache::getInstance()
{
	static SpineSkeletonCache internalShared;
	return &internalShared;
}

void SpineSkeletonCache::removeData(const char* dataFile)
{
	auto target = _cacheTable.find(dataFile);
	if (target != _cacheTable.end()) {
		target->second->release();
		_cacheTable.erase(target);
	}
}

SpineSkeletonCache::SkeletonData* SpineSkeletonCache::addData(const char* dataFile, const char* atlasFile, float scale)
{
	auto target = _cacheTable.find(dataFile);
	if (target != _cacheTable.end()) {
		target->second->retain();
		return target->second;
	}

	spSkeletonData* skeletonData = nullptr;
	spAttachmentLoader* loader = nullptr;
	bool ok = false;

	do {
		spAtlas* atlas = spAtlas_createFromFile(atlasFile, 0);

		if (nullptr == (atlas))
			break;

		loader = (spAttachmentLoader*)Cocos2dAttachmentLoader_create(atlas);

		spSkeletonJson* json = spSkeletonJson_createWithLoader(loader); // spSkeletonJson_create(atlas);
		if (nullptr == (json)) {
			spAtlas_dispose(atlas);
			break;
		}

		json->scale = scale;
		skeletonData = spSkeletonJson_readSkeletonDataFile(json, dataFile);

		int failed = 0;
		if ((loader->error1 != nullptr)) {
			++failed;
			error_log("#parse spine attachment failed, error:%s%s", loader->error1, loader->error2);
		}
		if ((json->error != nullptr)) {
			++failed;
			error_log("#parse spine .json data file failed, error:%s", json->error);
		}

		if (failed > 0) {
			if (skeletonData != nullptr)
				spSkeletonData_dispose(skeletonData);

			spSkeletonJson_dispose(json);
			spAtlas_dispose(atlas);
			spAttachmentLoader_dispose(loader);
			break;
		}

		spSkeletonJson_dispose(json); // json should be dispose, and atlas is used by shared attachment loader
		ok = true;
	} while (false);

	if (ok) {
		auto newData = new SkeletonData(skeletonData, loader);
		_cacheTable.insert(std::make_pair(dataFile, newData));
		newData->retain();
		return newData;
	}
	return nullptr;
}

void SpineSkeletonCache::removeAllData(void)
{
	for (auto & e : _cacheTable)
	{
		e.second->release();
	}
	_cacheTable.clear();
}

void SpineSkeletonCache::removeAllUnusedData(void)
{
	auto _First = _cacheTable.begin();
	auto _Last = _cacheTable.end();
	for (; _First != _Last; ) {
		if ((*_First).second->getReferenceCount() == 1) {
			(*_First).second->release();
			_cacheTable.erase(_First++);
			continue;
		}
		++_First;
	}
}

};

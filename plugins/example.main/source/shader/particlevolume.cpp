// volumetric shader example that accesses particles and displays its own preview
// absolutely not optimized for speed

#include "c4d.h"
#include "c4d_symbols.h"
#include "customgui_matpreview.h"
#include "main.h"

using namespace cinema;

struct PVRender
{
	PVRender();
	~PVRender();

	Vector* mp;
	Int32		count;
};

class ParticleVolume : public MaterialData
{
private:
	PVRender* render;

public:
	virtual	VOLUMEINFO GetRenderInfo		(BaseMaterial* mat);

	virtual	INITRENDERRESULT InitRender				(BaseMaterial* mat, const InitRenderStruct& irs);
	virtual	void FreeRender				(BaseMaterial* mat);

	virtual	void CalcSurface			(BaseMaterial* mat, VolumeData* vd);
	virtual	void CalcTransparency	(BaseMaterial* mat, VolumeData* vd);
	virtual	void CalcVolumetric		(BaseMaterial* mat, VolumeData* vd);
	virtual Bool Init(GeListNode* node, Bool isCloneInit);
	virtual Bool Message(GeListNode* node, Int32 type, void* data);

	static NodeData* Alloc() { return NewObjClear(ParticleVolume); }

protected:
	Bool highDensity;
};

Bool ParticleVolume::Init(GeListNode* node, Bool isCloneInit)
{
	highDensity = false;
	return true;
}

VOLUMEINFO ParticleVolume::GetRenderInfo(BaseMaterial* mat)
{
	return VOLUMEINFO::TRANSPARENCY | VOLUMEINFO::VOLUMETRIC;
}

#define MAX_PARTICLES 10000

PVRender::PVRender()
{
	mp = nullptr;
	count = 0;
}

PVRender::~PVRender()
{
	DeleteMem(mp);
}

static Bool FillPV(PVRender* pv, BaseObject* op)
{
	while (op)
	{
		if (op->GetType() == Oparticle && !op->GetDown() && op->GetDeformMode())	// particle system without geometry
		{
			ParticleObject* po = (ParticleObject*)op;
			ParticleTag*		pt = (ParticleTag*)op->GetTag(Tparticle);
			if (pt)
			{
				Vector*					mp = nullptr;
				const Particle* pp = nullptr;
				Int32 pcnt = po->GetParticleCount(), rcnt = 0, i;

				for (i = 0; i < pcnt; i++)
				{
					pp = po->GetParticleR(pt, i);
					if (pp->bits & (PARTICLEFLAGS::VISIBLE | PARTICLEFLAGS::ALIVE))
						rcnt++;
				}

				if (pv->count + rcnt > 0)
				{
					iferr (mp = NewMemClear(Vector, pv->count + rcnt))
						return false;

					CopyMemType(pv->mp, mp, pv->count);
					DeleteMem(pv->mp);
					pv->mp = mp;

					for (i = 0; i < pcnt; i++)
					{
						pp = po->GetParticleR(pt, i);
						if ((pp->bits & (PARTICLEFLAGS::VISIBLE | PARTICLEFLAGS::ALIVE)) == (PARTICLEFLAGS::VISIBLE | PARTICLEFLAGS::ALIVE))
							mp[pv->count++] = pp->off;
					}
				}
			}
		}

		if (!FillPV(pv, op->GetDown()))
			return false;
		op = op->GetNext();
	}
	return true;
}

INITRENDERRESULT ParticleVolume::InitRender(BaseMaterial* mat, const InitRenderStruct& irs)
{
	render = NewObjClear(PVRender);
	if (!render)
		return INITRENDERRESULT::OUTOFMEMORY;

	if (irs.doc && !FillPV(render, irs.doc->GetFirstObject()))
	{
		DeleteObj(render);
		return INITRENDERRESULT::OUTOFMEMORY;
	}

	return INITRENDERRESULT::OK;
}

void ParticleVolume::FreeRender(BaseMaterial* mat)
{
	DeleteObj(render);
}

static Bool SphereIntersection(const Vector& mp, Float rad, Ray* ray, Float maxdist, Float* length)
{
	Vector z;
	Float	 kr, ee, det, s, s1;

	kr	= rad * rad;
	z		= (Vector)ray->p - mp;
	ee	= Dot((Vector)ray->v, z);
	det = ee * ee - z.GetSquaredLength() + kr;

	if (det < 0.0)
		return false;

	det = Sqrt(det);
	s = det - ee;

	if (s <= 0.0)
		return false;

	s1 = -det - ee;

	if (s1 >= maxdist)
		return false;

	if (s >= maxdist)
		s = maxdist;
	if (s1 < 0.0)
		s1 = 0.0;

	*length = s - s1;

	return true;
}

void ParticleVolume::CalcSurface(BaseMaterial* mat, VolumeData* vd)
{
	vd->trans = Vector(1.0);
}

static Float GetCoverage(PVRender* pv, VolumeData* sd)
{
#define PT_RADIUS 50.0

	Int32 i;
	Float len, maxlen = 0.0;

	for (i = 0; i < pv->count; i++)
	{
		if (!SphereIntersection(pv->mp[i], PT_RADIUS, sd->ray, sd->dist, &len))
			continue;

		len	 = len / (2.0 * PT_RADIUS);
		len *= len;
		len *= len;

		maxlen = len + maxlen - len * maxlen;
	}

	if (maxlen > 1.0)
		maxlen = 1.0;
	return maxlen;
}

void ParticleVolume::CalcVolumetric(BaseMaterial* mat, VolumeData* vd)
{
	Float cov = GetCoverage(render, vd);
	vd->col = Vector(0.0, 0.0, cov);
	vd->trans = Vector(1.0 - cov);
}

void ParticleVolume::CalcTransparency(BaseMaterial* mat, VolumeData* vd)
{
	Float cov = GetCoverage(render, vd);
	vd->trans = Vector(1.0 - cov);
}

Bool ParticleVolume::Message(GeListNode* node, Int32 type, void* data)
{
	switch (type)
	{
		case MATPREVIEW_GET_OBJECT_INFO:
			// the preview needs our object information
		{
			MatPreviewObjectInfo* info = (MatPreviewObjectInfo*)data;
			info->bHandlePreview = true;												// own preview handling
			info->bNeedsOwnScene = true;												// we need our own entry in the preview scene cache
			info->bNoStandardScene = true;											// create our own preview scene
			info->lFlags = MATPREVIEW_FLAG_HIDE_SCENE_SETTINGS;	// hide the scene settings entry
			return true;
			break;
		}
		case MATPREVIEW_MODIFY_CACHE_SCENE:
			// modify the preview scene here. We have a pointer to a scene inside the preview scene cache.
			// We set bNoStandardScene = true, so the passed scene is an empty document
		{
			MatPreviewModifyCacheScene* scene = (MatPreviewModifyCacheScene*)data;
			// add an environment object and create a texture tag
			BaseObject* env = BaseObject::Alloc(Oenvironment);
			if (!env)
				return false;
			env->SetName("environment"_s);
			BaseTag* mattag = env->MakeTag(Ttexture);
			if (!mattag)
				return false;
			scene->pDoc->InsertObject(env, nullptr, nullptr);
			// add a particle emitter
			BaseObject* emitter = BaseObject::Alloc(Oparticle);
			if (!emitter)
				return false;
			emitter->SetName("emitter"_s);
			scene->pDoc->InsertObject(emitter, nullptr, nullptr);
			// get the current camera and change the position
			BaseObject* cam = scene->pDoc->GetActiveBaseDraw() ? scene->pDoc->GetActiveBaseDraw()->GetEditorCamera() : nullptr;
			if (cam)
			{
				cam->SetRelPos(Vector(0, 0, -150));
				cam->SetRelRot(Vector(0, 0, 0));
			}
			return true;
			break;
		}
		case MATPREVIEW_PREPARE_SCENE:
		{
			// this is called each time the preview wants to render our scene
			MatPreviewPrepareScene* preparescene = (MatPreviewPrepareScene*)data;

			AutoAlloc<AliasTrans> trans;
			if (!trans)
				return false;
			if (!trans->Init(GetActiveDocument()))
				return false;
			BaseMaterial* matclone = (BaseMaterial*)(Get()->GetClone(COPYFLAGS::NONE, trans));
			preparescene->pDoc->InsertMaterial(matclone);
			trans->Translate(true);
			if (preparescene->pLink)
				preparescene->pLink->SetLink(matclone);		// necessary

			// find the environment object and set the material
			BaseObject* env = preparescene->pDoc->SearchObject("environment"_s);
			if (env)
			{
				TextureTag* textag = (TextureTag*)env->GetTag(Ttexture);
				if (textag)
					textag->SetMaterial(matclone);
			}

			// change the emitter data depending on the highDensity settings
			BaseObject* emitter = preparescene->pDoc->SearchObject("emitter"_s);
			if (emitter)
			{
				BaseContainer* localData = emitter->GetDataInstance();
				localData->SetInt32(PARTICLEOBJECT_BIRTHRAYTRACER, highDensity ? 10 : 5);
			}

			preparescene->bScenePrepared = true;	// inform the preview that the scene is prepared now
			return true;
			break;
		}
		case MATPREVIEW_GET_POPUP_OPTIONS:
		{
			BaseContainer* bc = (BaseContainer*)data;
			bc->SetString(MATPREVIEW_POPUP_NAME, GeLoadString(IDS_PARTICLEVOLUME));												// element with index MATPREVIEW_POPUP_NAME is the text of the popup item
			bc->SetString(1, GeLoadString(IDS_PARTICLEVOLUME_HIGH_DENSITY) + (highDensity ? "&c&" : ""));	// container entries should start with 1
			bc->SetString(0, String());																																					// add an example separator
			bc->SetString(2, GeLoadString(IDS_PARTICLEVOLUME_LOW_DENSITY) + (!highDensity ? "&c&" : ""));
			return true;
			break;
		}
		case MATPREVIEW_HANDLE_POPUP_MSG:
		{
			Int32 l = *static_cast<Int32*>(data);
			if (l == 1)
				highDensity = true;
			else if (l == 2)
				highDensity = false;
			return true;
			break;
		}
		case MATPREVIEW_GENERATE_IMAGE:
		{
			MatPreviewGenerateImage* image = (MatPreviewGenerateImage*)data;

			if (image->pDoc)
			{
				Int32					w = image->pDest->GetBw();
				Int32					h = image->pDest->GetBh();
				BaseContainer bcRender = image->pDoc->GetActiveRenderData()->GetDataInstanceRef();
				bcRender.SetFloat(RDATA_XRES, w);
				bcRender.SetFloat(RDATA_YRES, h);
				bcRender.SetInt32(RDATA_ANTIALIASING, ANTI_GEOMETRY);

				image->pDest->Clear(0, 0, 0);
				BaseTime bt(image->rTime + 1.0);	// add one second that some particles are produced
				image->pDoc->SetTime(bt);
				image->pDoc->ExecutePasses(image->pThread, true, true, false, BUILDFLAGS::NONE);
				image->lResult = RenderDocument(image->pDoc, bcRender, nullptr, nullptr, image->pDest,
													 RENDERFLAGS::EXTERNAL | RENDERFLAGS::PREVIEWRENDER, image->pThread);
			}
			return true;
			break;
		}
	}
	return MaterialData::Message(node, type, data);
}


// be sure to use a unique ID obtained from developers.maxon.net
#define ID_PARTICLEVOLUME	1001163

Bool RegisterParticleVolume()
{
	String name = GeGetDefaultFilename(DEFAULTFILENAME_SHADER_VOLUME) + GeLoadString(IDS_PARTICLEVOLUME);	// place in default Shader section
	return RegisterMaterialPlugin(ID_PARTICLEVOLUME, name, 0, ParticleVolume::Alloc, "MParticleVolume"_s, 0);
}

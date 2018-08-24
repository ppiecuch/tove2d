/*************************************************************************/
/*  vg_path.cpp  	                                                     */
/*************************************************************************/

#include "vg_path.h"
#include "os/file_access.h"
#include "editor/editor_node.h"
#include "vg_color.h"
#include "vg_adaptive_renderer.h"

inline Rect2 tove_bounds_to_rect2(const float *bounds) {
	return Rect2(bounds[0], bounds[1],  bounds[2] - bounds[0], bounds[3] - bounds[1]);
}

static tove::PaintRef to_tove_paint(Ref<VGPaint> p_paint) {
	Ref<VGColor> color = p_paint;
	if (color.is_null()) {
		return tove::PaintRef();
	} else {
		const Color c = color->get_color();
		return tove::tove_make_shared<tove::Color>(c.r, c.g, c.b, c.a);
	}	
}

static Ref<VGPaint> from_tove_paint(const tove::PaintRef &p_paint) {
	if (p_paint) {
		ToveRGBA rgba;
		p_paint->getRGBA(rgba, 1.0f);
		Ref<VGColor> color;
		color.instance();
		color->set_color(Color(rgba.r, rgba.g, rgba.b, rgba.a));
		return color;
	} else {
		return Ref<VGPaint>();
	}
}

static Point2 compute_center(const tove::PathRef &p_path) {
	float x = 0.0f, y = 0.0f;
	int k = 0;
	const int n = p_path->getNumSubpaths();
	for (int i = 0; i < n; i++) {
		tove::SubpathRef tove_subpath = p_path->getSubpath(i);
		const int m = tove_subpath->getNumPoints();
		const float *p = tove_subpath->getPoints();
		for (int j = 0; j < m; j++) {
			if (k++ == 0) {
				x = p[2 * j + 0];
				y = p[2 * j + 1];
			} else {
				x += p[2 * j + 0];
				y += p[2 * j + 1];
			}
		}
	}
	if (k == 0) {
		return Point2(0, 0);
	}
	return Point2(x / k, y / k);
}

tove::GraphicsRef VGPath::create_tove_graphics() const {
	tove::GraphicsRef tove_graphics = tove::tove_make_shared<tove::Graphics>();
	tove_graphics->addPath(tove_path);
	return tove_graphics;
}

void VGPath::add_tove_path(const tove::GraphicsRef &p_tove_graphics) const {
	const Transform2D t = get_transform();

	const Vector2 &tx = t.elements[0];
	const Vector2 &ty = t.elements[1];
	const Vector2 &to = t.elements[2];
	tove::nsvg::Transform tove_transform(tx.x, ty.x, to.x, tx.y, ty.y, to.y);

	const tove::PathRef transformed_path = tove::tove_make_shared<tove::Path>();
	transformed_path->set(tove_path, tove_transform);
	p_tove_graphics->addPath(transformed_path);
}

void VGPath::set_inherited_dirty(Node *p_node) {
	const int n = p_node->get_child_count();
	for (int i = 0; i < n; i++) {
		Node *child = p_node->get_child(i);
		if (child->is_class_ptr(get_class_ptr_static())) {
			VGPath *path = Object::cast_to<VGPath>(child);
			if (path->inherits_renderer()) {
				path->set_dirty();
				set_inherited_dirty(path);
			}
		} else {
			set_inherited_dirty(child);
		}
	}
}

void VGPath::compose_graphics(const tove::GraphicsRef &p_tove_graphics, Node *p_node) {
	const int n = p_node->get_child_count();
	for (int i = 0; i < n; i++) {
		Node *child = p_node->get_child(i);
		if (child->is_class_ptr(get_class_ptr_static())) {
			Object::cast_to<VGPath>(child)->add_tove_path(p_tove_graphics);
		} else {
			compose_graphics(p_tove_graphics, child);
		}
	}
}

bool VGPath::inherits_renderer() const {
	return renderer.is_null();
}

Ref<VGRenderer> VGPath::get_inherited_renderer() const {
	if (!inherits_renderer()) {
		return renderer;
	}

	Node *node = get_parent();
	while (node) {
		if (node->is_class_ptr(get_class_ptr_static())) {
			VGPath *path = Object::cast_to<VGPath>(node);
			if (path->renderer.is_valid()) {
				return path->renderer;
			}
		}
		node = node->get_parent();
	}
	return Ref<VGRenderer>();
}

void VGPath::update_mesh_representation() {

	if (!dirty) {
		return;
	}
	dirty = false;
	bounds = Rect2();

	if (!is_empty()) {
		Ref<VGRenderer> renderer = get_inherited_renderer();
		if (renderer.is_valid()) {
			tove::GraphicsRef tove_graphics = create_tove_graphics();
			// FIXME use composite graphics from root node here.
			mesh_data = renderer->graphics_to_mesh(tove_graphics);
			bounds = tove_bounds_to_rect2(tove_graphics->getBounds());
		}
	}		
}

void VGPath::update_tove_fill_color() {
	tove_path->setFillColor(to_tove_paint(fill_color));
}

void VGPath::update_tove_line_color() {
	tove_path->setLineColor(to_tove_paint(line_color));
}

void VGPath::create_fill_color() {
	set_fill_color(from_tove_paint(tove_path->getFillColor()));
}

void VGPath::create_line_color() {
	set_line_color(from_tove_paint(tove_path->getLineColor()));
}

void VGPath::_notification(int p_what) {

	switch (p_what) {
		case NOTIFICATION_DRAW: {
			update_mesh_representation();
			if (!is_empty()) {
				draw_mesh(mesh_data.mesh, mesh_data.texture, Ref<Texture>());
			}
		} break;
		case NOTIFICATION_PARENTED: {
			if (inherits_renderer()) {
				set_dirty();
				set_inherited_dirty(this);
			}
		} break;
	}
}

void VGPath::_bind_methods() {

	ClassDB::bind_method(D_METHOD("set_renderer", "renderer"), &VGPath::set_renderer);
	ClassDB::bind_method(D_METHOD("get_renderer"), &VGPath::get_renderer);

	ClassDB::bind_method(D_METHOD("set_fill_color", "color"), &VGPath::set_fill_color);
	ClassDB::bind_method(D_METHOD("get_fill_color"), &VGPath::get_fill_color);

	ClassDB::bind_method(D_METHOD("set_line_color", "color"), &VGPath::set_line_color);
	ClassDB::bind_method(D_METHOD("get_line_color"), &VGPath::get_line_color);

	ClassDB::bind_method(D_METHOD("set_line_width", "width"), &VGPath::set_line_width);
	ClassDB::bind_method(D_METHOD("get_line_width"), &VGPath::get_line_width);

	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "renderer", PROPERTY_HINT_RESOURCE_TYPE, "VGRenderer"), "set_renderer", "get_renderer");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "fill_color", PROPERTY_HINT_RESOURCE_TYPE, "VGPaint"), "set_fill_color", "get_fill_color");
	ADD_PROPERTY(PropertyInfo(Variant::OBJECT, "line_color", PROPERTY_HINT_RESOURCE_TYPE, "VGPaint"), "set_line_color", "get_line_color");
	ADD_PROPERTY(PropertyInfo(Variant::REAL, "line_width", PROPERTY_HINT_RANGE, "0,100,0.01"), "set_line_width", "get_line_width");

	ClassDB::bind_method(D_METHOD("insert_curve", "subpath", "t"), &VGPath::insert_curve);
	ClassDB::bind_method(D_METHOD("remove_curve", "subpath", "curve"), &VGPath::remove_curve);
	ClassDB::bind_method(D_METHOD("set_points", "subpath", "points"), &VGPath::set_points);
}

bool VGPath::_set(const StringName &p_name, const Variant &p_value) {
	String name = p_name;
	set_dirty();

	if (name == "name") {
		String s = p_value;
		CharString t = s.utf8();
		tove_path->setName(t.get_data());
	} else if (name == "line_width") {
		tove_path->setLineWidth(p_value);
	} else if (name == "fill_rule") {
		if (p_value == "nonzero") {
			tove_path->setFillRule(TOVE_FILLRULE_NON_ZERO);
		} else if (p_value == "evenodd") {
			tove_path->setFillRule(TOVE_FILLRULE_EVEN_ODD);
		} else {
			return false;
		}
	} else if (name.begins_with("subpaths/")) {
		int subpath = name.get_slicec('/', 1).to_int();
		String subwhat = name.get_slicec('/', 2);

		if (tove_path->getNumSubpaths() == subpath) {
			tove::SubpathRef tove_subpath = tove::tove_make_shared<tove::Subpath>();
			tove_path->addSubpath(tove_subpath);
		}

		ERR_FAIL_INDEX_V(subpath, tove_path->getNumSubpaths(), false);
		tove::SubpathRef tove_subpath = tove_path->getSubpath(subpath);
		
		if (subwhat == "closed") {
			tove_subpath->setIsClosed(p_value);
		} if (subwhat == "points") {
			PoolVector2Array pts = p_value;
			const int n = pts.size();
			float *buf = new float[n * 2];
			for (int i = 0; i < n; i++) {
				const Vector2 &p = pts[i];
				buf[2 * i + 0] = p.x;
				buf[2 * i + 1] = p.y;
			}
			tove_subpath->setPoints(buf, n, false);
			delete[] buf;
		} else {
			return false;
		}
	} else {
		return false;
	}

	return true;	
}

bool VGPath::_get(const StringName &p_name, Variant &r_ret) const {
	String name = p_name;

	if (name == "name") {
		r_ret = String::utf8(tove_path->getName());
	} else if (name == "line_width") {
		r_ret = tove_path->getLineWidth();
	} else if (name == "fill_rule") {
		switch (tove_path->getFillRule()) {
			case TOVE_FILLRULE_NON_ZERO: {
				r_ret = "nonzero";
			} break;
			case TOVE_FILLRULE_EVEN_ODD: {
				r_ret = "evenodd";
			} break;
			default: {
				return false;
			} break;
		}
	} else if (name.begins_with("subpaths/")) {
		int subpath = name.get_slicec('/', 1).to_int();
		String subwhat = name.get_slicec('/', 2);
		ERR_FAIL_INDEX_V(subpath, tove_path->getNumSubpaths(), false);
		tove::SubpathRef tove_subpath = tove_path->getSubpath(subpath);
		if (subwhat == "closed") {
			r_ret = tove_subpath->isClosed();
		} else if (subwhat == "points") {
			const int n = tove_subpath->getNumPoints();
			PoolVector2Array out;
			out.resize(n);
			{
				PoolVector2Array::Write w = out.write();
				const float *pts = tove_subpath->getPoints();
				for (int i = 0; i < n; i++) {
					w[i] = Vector2(pts[2 * i + 0], pts[2 * i + 1]);
				}
			}
			r_ret = out;
		} else {
			return false;
		}
	} else {
		return false;
	}

	return true;
}

void VGPath::_get_property_list(List<PropertyInfo> *p_list) const {

	p_list->push_back(PropertyInfo(Variant::STRING, "name", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL));
	p_list->push_back(PropertyInfo(Variant::REAL, "line_width", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL));
	p_list->push_back(PropertyInfo(Variant::INT, "fill_rule", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL));

	for (int j = 0; j < tove_path->getNumSubpaths(); j++) {
		const String subpath_prefix = "subpaths/" + itos(j);

		p_list->push_back(PropertyInfo(Variant::BOOL, subpath_prefix + "/closed", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL));
		p_list->push_back(PropertyInfo(Variant::POOL_VECTOR2_ARRAY, subpath_prefix + "/points", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_NOEDITOR | PROPERTY_USAGE_INTERNAL));			
	}
}

Ref<VGRenderer> VGPath::get_renderer() {
	return renderer;
}

void VGPath::set_renderer(const Ref<VGRenderer> &p_renderer) {

	renderer = p_renderer;
	set_inherited_dirty(this);
	set_dirty();
}

Ref<VGPaint> VGPath::get_fill_color() const {
	return fill_color;
}

void VGPath::set_fill_color(const Ref<VGPaint> &p_paint) {

	if (fill_color == p_paint) {
		return;
	}

	if (fill_color.is_valid()) {
		fill_color->remove_change_receptor(this);
	}
	fill_color = p_paint;
	if (fill_color.is_valid()) {
		fill_color->add_change_receptor(this);
	}

	update_tove_fill_color();
	set_dirty();
}

Ref<VGPaint> VGPath::get_line_color() const {
	return line_color;
}

void VGPath::set_line_color(const Ref<VGPaint> &p_paint) {

	if (line_color == p_paint) {
		return;
	}

	if (line_color.is_valid()) {
		line_color->remove_change_receptor(this);
	}
	line_color = p_paint;
	if (line_color.is_valid()) {
		line_color->add_change_receptor(this);
	}

	update_tove_line_color();
	set_dirty();
}

float VGPath::get_line_width() const {
	return tove_path->getLineWidth();
}

void VGPath::set_line_width(const float p_line_width) {
	tove_path->setLineWidth(p_line_width);
	set_dirty();
}

bool VGPath::is_inside(const Point2 &p_point) const {
	return tove_path->isInside(p_point.x, p_point.y);
}

VGPath *VGPath::find_clicked_child(const Point2 &p_point) {
	const int n = get_child_count();
	for (int i = 0; i < n; i++) {
		Node *child = get_child(i);
		if (child->is_class_ptr(get_class_ptr_static())) {
			VGPath *path = Object::cast_to<VGPath>(child);
			if (path->is_visible()) {
				Point2 p = path->get_transform().affine_inverse().xform(p_point);
				if (path->is_inside(p)) {
					return path;
				}
			}
		}
	}
	
	return nullptr;	
}

bool VGPath::_is_clicked(const Point2 &p_point) const {
	if (is_inside(p_point)) {
		return true;
	}

	const int n = get_child_count();
	for (int i = 0; i < n; i++) {
		Node *child = get_child(i);
		if (child->is_class_ptr(get_class_ptr_static())) {
			if (Object::cast_to<VGPath>(child)->_is_clicked(p_point)) {
				return true;
			}
		}
	}
	
	return false;
}

Rect2 VGPath::_edit_get_rect() const {
	const_cast<VGPath*>(this)->update_mesh_representation();

	const int n = get_child_count();
	for (int i = 0; i < n; i++) {
		Node *child = get_child(i);
		if (child->is_class_ptr(get_class_ptr_static())) {
			// FIXME: apply child transform
			bounds.merge(Object::cast_to<VGPath>(child)->_edit_get_rect());
		}
	}

	return bounds;
}

bool VGPath::_edit_is_selected_on_click(const Point2 &p_point, double p_tolerance) const {
	if (get_parent() && get_parent()->is_class_ptr(get_class_ptr_static())) {
		return false;
	}

	return _is_clicked(p_point);
}

void VGPath::_changed_callback(Object *p_changed, const char *p_prop) {

	if (fill_color.ptr() == p_changed) {
		update_tove_fill_color();
		set_dirty();
	}

	if (line_color.ptr() == p_changed) {
		update_tove_line_color();
		set_dirty();
	}
}

void VGPath::set_points(int p_subpath, Array p_points) {
	ERR_FAIL_INDEX(p_subpath, tove_path->getNumSubpaths());
	tove::SubpathRef tove_subpath = tove_path->getSubpath(p_subpath);
	const int n = p_points.size();
	float *p = new float[2 * n];
	for (int i = 0; i < n; i++) {
		const Vector2 q = p_points[i];
		p[2 * i + 0] = q.x;
		p[2 * i + 1] = q.y;
	}
	try {
		tove_subpath->setPoints(p, n, false);
		set_dirty();
	} catch(...) {
		delete[] p;
		throw;	
	}
	delete[] p;
}

void VGPath::insert_curve(int p_subpath, float p_t) {
	ERR_FAIL_INDEX(p_subpath, tove_path->getNumSubpaths());
	tove::SubpathRef tove_subpath = tove_path->getSubpath(p_subpath);
	tove_subpath->insertCurveAt(p_t);
	set_dirty();
}

void VGPath::remove_curve(int p_subpath, int p_curve) {
	ERR_FAIL_INDEX(p_subpath, tove_path->getNumSubpaths());
	tove::SubpathRef tove_subpath = tove_path->getSubpath(p_subpath);
	tove_subpath->removeCurve(p_curve);
	set_dirty();
}

void VGPath::set_dirty() {
	dirty = true;
	update();
}

bool VGPath::is_empty() const {
	const int n = tove_path->getNumSubpaths();
	for (int i = 0; i < n; i++) {
		if (tove_path->getSubpath(i)->getNumPoints() > 0) {
			return false;
		}
	}
	return true;
}

int VGPath::get_num_subpaths() const {
	return tove_path->getNumSubpaths();
}

tove::SubpathRef VGPath::get_subpath(int p_subpath) const {
	return tove_path->getSubpath(p_subpath);
}

tove::PathRef VGPath::get_tove_path() const {
	return tove_path;
}

MeshInstance2D *VGPath::create_mesh_node() {
	const Ref<VGRenderer> renderer = get_inherited_renderer();
	tove::GraphicsRef tove_graphics = tove::tove_make_shared<tove::Graphics>();
	compose_graphics(tove_graphics, this);
	MeshInstance2D *mesh_inst = memnew(MeshInstance2D);
	if (renderer.is_valid()) {
		VGMeshData mesh_data = renderer->graphics_to_mesh(tove_graphics);
		mesh_inst->set_mesh(mesh_data.mesh);
		mesh_inst->set_texture(mesh_data.texture);		
	}
	return mesh_inst;
}

VGPath::VGPath() {
	tove_path = tove::tove_make_shared<tove::Path>();

	/*tove::SubpathRef tove_subpath = tove::tove_make_shared<tove::Subpath>();
	tove_subpath->drawEllipse(0, 0, 100, 100);
	tove_path->addSubpath(tove_subpath);

	tove_path->setFillColor(tove::tove_make_shared<tove::Color>(0.8, 0.1, 0.1));
	tove_path->setLineColor(tove::tove_make_shared<tove::Color>(0, 0, 0));
	create_fill_color();
	create_line_color();*/

	set_dirty();
}

VGPath::VGPath(tove::PathRef p_path) {
	tove_path = p_path;
	create_fill_color();
	create_line_color();
	set_dirty();
}

VGPath::~VGPath() {
	if (fill_color.is_valid()) {
		fill_color->remove_change_receptor(this);
	}
	if (line_color.is_valid()) {
		line_color->remove_change_receptor(this);
	}
}

VGPath *VGPath::createFromSVG(Ref<Resource> p_resource, Node *p_owner) {

	if (!p_resource->get_path().ends_with(".svg")) {
		return NULL;
	}

	String path = p_resource->get_path();
	String units = "px";
	float dpi = 96.0;

	Vector<uint8_t> buf = FileAccess::get_file_as_array(path);

	String str;
	str.parse_utf8((const char *)buf.ptr(), buf.size());

	tove::GraphicsRef tove_graphics = tove::Graphics::createFromSVG(
		str.utf8().ptr(), units.utf8().ptr(), dpi);

	VGPath *root = memnew(VGPath(tove::tove_make_shared<tove::Path>()));
	root->set_owner(p_owner);

	Ref<VGAdaptiveRenderer> renderer;
	renderer.instance();
	root->renderer = renderer;

	const float *bounds = tove_graphics->getBounds();

	float s = 256.0f / MAX(bounds[2] - bounds[0], bounds[3] - bounds[1]);
	if (s > 1.0f) {
		tove::nsvg::Transform transform(s, 0, 0, 0, s, 0);
		transform.setWantsScaleLineWidth(true);
		tove_graphics->set(tove_graphics, transform);
	}

	const int n = tove_graphics->getNumPaths();
	for (int i = 0; i < n; i++) {
		tove::PathRef tove_path = tove_graphics->getPath(i);
		Point2 center = compute_center(tove_path);
		tove_path->set(tove_path, tove::nsvg::Transform(1, 0, -center.x, 0, 1, -center.y));

		VGPath *path = memnew(VGPath(tove_path));
		path->set_position(center);

		std::string name = tove_path->getName();
		if (name.empty()) {
			name = "path";
		}

//		Point2 center = compute_center(tove_path);
//		path->set_position(center);

/*
		Transform2D t = path->get_transform().affine_inverse();
		const Vector2 &tx = t.elements[0];
		const Vector2 &ty = t.elements[1];
		const Vector2 &to = t.elements[2];

		printf("?? %f %f\n", center.x, center.y);
		printf("! %f %f %f %f %f %f\n", tx.x, ty.x, to.x, tx.y, ty.y, to.y);

		//tove::nsvg::Transform tove_transform(tx.x, ty.x, to.x, tx.y, ty.y, to.y);#
		tove::nsvg::Transform tove_transform(1, 0, -center.x, 0, 1, -center.y);
		tove_path->set(tove_path, tove_transform);
		*/
//		tove_path->set(tove_path, tove::nsvg::Transform(1, 0, -center.x, 0, 1, -center.y));

		//path->get_transform().elements[0]

		path->set_name(String(name.c_str()));
		root->add_child(path);
		path->_set_owner_nocheck(p_owner); // our parent is orphaned currently.
	}

	//clip_set = tove_graphics->getClipSet();
	//set_dirty();

	return root;
}

Node *createVectorSprite(Ref<Resource> p_resource, Node *p_owner) {
	return VGPath::createFromSVG(p_resource, p_owner);
}

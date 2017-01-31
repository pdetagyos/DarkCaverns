/*
    In-Game Screen

    Written by Peter de Tagyos
    Started: 1/30/2017
*/

#define STATS_WIDTH		20
#define STATS_HEIGHT 	5

#define LOG_WIDTH		58
#define LOG_HEIGHT		5

#define INVENTORY_LEFT		20
#define INVENTORY_TOP		7
#define INVENTORY_WIDTH		40
#define INVENTORY_HEIGHT	30


internal void render_game_map_view(Console *console);
internal void render_message_log_view(Console *console);
internal void render_stats_view(Console *console);


// Init / Show screen --

internal UIScreen * 
screen_show_in_game() {
	List *igViews = list_new(NULL);

	UIRect mapRect = {0, 0, (16 * MAP_WIDTH), (16 * MAP_HEIGHT)};
	UIView *mapView = view_new(mapRect, MAP_WIDTH, MAP_HEIGHT, 
							   "./terminal16x16.png", 0, render_game_map_view);
	list_insert_after(igViews, NULL, mapView);

	UIRect statsRect = {0, (16 * MAP_HEIGHT), (16 * STATS_WIDTH), (16 * STATS_HEIGHT)};
	UIView *statsView = view_new(statsRect, STATS_WIDTH, STATS_HEIGHT,
								 "./terminal16x16.png", 0, render_stats_view);
	list_insert_after(igViews, NULL, statsView);

	UIRect logRect = {(16 * 22), (16 * MAP_HEIGHT), (16 * LOG_WIDTH), (16 * LOG_HEIGHT)};
	UIView *logView = view_new(logRect, LOG_WIDTH, LOG_HEIGHT,
							   "./terminal16x16.png", 0, render_message_log_view);
	list_insert_after(igViews, NULL, logView);

	UIScreen *inGameScreen = malloc(sizeof(UIScreen));
	inGameScreen->views = igViews;
	inGameScreen->activeView = mapView;

	return inGameScreen;
}


// Render Functions --

internal void 
render_game_map_view(Console *console) {
	// Setup layer render history  
	u8 layerRendered[MAP_WIDTH][MAP_HEIGHT];
	for (u32 x = 0; x < MAP_WIDTH; x++) {
		for (u32 y = 0; y < MAP_HEIGHT; y++) {
			layerRendered[x][y] = LAYER_UNSET;
		}
	}

	ListElement *e = list_head(visibilityComps);
	while (e != NULL) {
		Visibility *vis = (Visibility *)list_data(e);
		Position *p = (Position *)game_object_get_component(&gameObjects[vis->objectId], COMP_POSITION);
		if (fovMap[p->x][p->y] > 0) {
			vis->hasBeenSeen = true;
			// Don't render if we've already written something to to this cell at a higher layer
			if (p->layer > layerRendered[p->x][p->y]) {
				console_put_char_at(console, vis->glyph, p->x, p->y, vis->fgColor, vis->bgColor);
				layerRendered[p->x][p->y] = p->layer;
			}

		} else if (vis->visibleOutsideFOV && vis->hasBeenSeen) {
			u32 fullColor = vis->fgColor;
			u32 fadedColor = COLOR_FROM_RGBA(RED(fullColor), GREEN(fullColor), BLUE(fullColor), 0x77);
			// Don't render if we've already written something to to this cell at a higher layer
			if (p->layer > layerRendered[p->x][p->y]) {
				console_put_char_at(console, vis->glyph, p->x, p->y, fadedColor, 0x000000FF);
				layerRendered[p->x][p->y] = p->layer;
			}
		}
		e = list_next(e);
	}

}

internal void 
render_inventory_view(Console *console) {

	UIRect rect = {0, 0, INVENTORY_WIDTH, INVENTORY_HEIGHT};
	view_draw_rect(console, &rect, 0x222222FF, 1, 0xFF990099);

}

internal void 
render_message_log_view(Console *console) {

	UIRect rect = {0, 0, LOG_WIDTH, LOG_HEIGHT};
	view_draw_rect(console, &rect, 0x191919FF, 0, 0xFF990099);

	if (messageLog == NULL) { return; }

	// Get the last 5 messages from the log
	ListElement *e = list_tail(messageLog);
	i32 msgCount = list_size(messageLog);
	u32 row = 4;
	u32 col = 0;

	if (msgCount < 5) {
		row -= (5 - msgCount);
	} else {
		msgCount = 5;
	}

	for (i32 i = 0; i < msgCount; i++) {
		if (e != NULL) {
			Message *m = (Message *)list_data(e);
			UIRect rect = {.x = col, .y = row, .w = LOG_WIDTH, .h = 1};
			console_put_string_in_rect(console, m->msg, rect, false, m->fgColor, 0x00000000);
			e = list_prev(e);			
			row -= 1;
		}
	}
}

internal void 
render_stats_view(Console *console) {
	
	UIRect rect = {0, 0, STATS_WIDTH, STATS_HEIGHT};
	view_draw_rect(console, &rect, 0x222222FF, 0, 0xFF990099);

	// HP health bar
	Health *playerHealth = game_object_get_component(player, COMP_HEALTH);
	console_put_char_at(console, 'H', 0, 1, 0xFF990099, 0x00000000);
	console_put_char_at(console, 'P', 1, 1, 0xFF990099, 0x00000000);
	i32 leftX = 3;
	i32 barWidth = 16;

	i32 healthCount = ceil(((float)playerHealth->currentHP / (float)playerHealth->maxHP) * barWidth);
	for (i32 x = 0; x < barWidth; x++) {
		if (x < healthCount) {
			console_put_char_at(console, 176, leftX + x, 1, 0x009900FF, 0x00000000);		
			console_put_char_at(console, 3, leftX + x, 1, 0x009900FF, 0x00000000);		
		} else {
			console_put_char_at(console, 176, leftX + x, 1, 0xFF990099, 0x00000000);		
		}
	}

	Combat *playerCombat = game_object_get_component(player, COMP_COMBAT);
	char *att = NULL;
	sasprintf(att, "ATT: %d (%d)", playerCombat->attack, playerCombat->attackModifier);
	console_put_string_at(console, att, 0, 2, 0xe6e600FF, 0x00000000);
	free(att);

	char *def = NULL;
	sasprintf(def, "DEF: %d (%d)", playerCombat->defense, playerCombat->defenseModifier);
	console_put_string_at(console, def, 0, 3, 0xe6e600FF, 0x00000000);
	free(def);

}


// Event Handling --

internal void 
show_inventory_overlay(UIScreen *screen) {

	UIRect overlayRect = {(16 * INVENTORY_LEFT), (16 * INVENTORY_TOP), (16 * INVENTORY_WIDTH), (16 * INVENTORY_HEIGHT)};
	UIView *overlayView = view_new(overlayRect, INVENTORY_WIDTH, INVENTORY_HEIGHT, 
							   "./terminal16x16.png", 0, render_inventory_view);
	list_insert_after(screen->views, list_tail(screen->views), overlayView);

}

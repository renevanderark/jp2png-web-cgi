(function ( $ ) {
	$.fn.jp2Viewer = function(filename, opts) {
		var canvas = this.get(0);
		var ctx = canvas.getContext('2d');
		var dataType = opts.dataType || "json";
		var primary = opts.primary || "http://" + window.location.hostname;
		var workers = opts.workers || [{address: primary, cores: 1}]; 
		var initScale = opts.initScale || "full-width";

		var jp2Header = false;
		var tiles = {};

		var scale;
		var reduction;
		var reductionScale;
		var currentWorker = 0;
		var xPos = 0;
		var yPos = 0;
		var curX;
		var curY;
		var dragging = false;
		var tiles2go = 1;

		this.on("mousedown", function(e) {
			dragging = true;
			curX = e.clientX;
			curY = e.clientY;
		});

		this.on("mousemove", function(e) {
			if(dragging) {
				var movX = curX - e.clientX;
				var movY = curY - e.clientY;
				xPos -= movX;
				yPos -= movY;

				ensureBounds();
				showImage();
				curX = e.clientX;
				curY = e.clientY;
			}
		});

		this.on("mouseup mouseout", function(e) {
			dragging = false;
		});

		this.on("mousewheel", function(e, delta, deltaX, deltaY) {
			if(delta > 0) {
				initScale = scale * 1.1;
				initialize(jp2Header);
			} else if(delta < 0) {
				initScale = scale * 0.9;
				initialize(jp2Header);
			}
		});

		function ensureBounds() {
			if(xPos + (jp2Header.x1 * scale) <= canvas.width) { xPos = canvas.width - (jp2Header.x1 * scale); }
			if(yPos + (jp2Header.y1 * scale) <= canvas.height) { yPos = canvas.height - (jp2Header.y1 * scale); }
			if(xPos > 0) { xPos = 0; }
			if(yPos > 0) { yPos = 0; }
			if(jp2Header.x1 * scale <= canvas.width) { 
				xPos = Math.floor((canvas.width - jp2Header.x1 * scale) / 2);
			}
		}

		function reduce(val, reduction) {
			if(reduction == 0) { return val; }
			return reduce(val / 2, --reduction);
		}

		function setScale(s) {
			switch(s) {
				case "full-width": scale =  canvas.width / jp2Header.x1; break;
				case "full-height": scale = canvas.height / jp2Header.y1; break;
				default: 
					scale = parseFloat(s);
					if(scale == 0.0) { scale = 1.0; }
			}
		}

		function setReduction(s) {
			var s = s || scale;
			reduction = jp2Header.num_res;
			while(--reduction) {
				if(reduce(1.0, reduction) > s) { 
					break; 
				}
			}
		}

		function initializeTiles() {
			for(var i = 0; i < jp2Header.num_res; i++) {
				tiles["redux-" + i] = [];
			}
		}

		function drawTile(tile) {
			if(tile.img.complete) {
				tiles2go--;
				ctx.drawImage(tile.img, 0,0, tile.img.width, tile.img.height, tile.dims.x, tile.dims.y, Math.ceil(tile.img.width * tile.dims.s), Math.ceil(tile.img.height * tile.dims.s));
			} else {
				setTimeout(function() { drawTile(tile); }, 1);
			}
			if(tiles2go == 0) { tiles2go = 1; showImage();}
		}

		function showImage() {
			var tileS = scale / reduce(1.0, reduction);
			var tilesX = Math.ceil(canvas.width / (jp2Header.tdx * scale)) + 1;
			var tilesY = Math.ceil(canvas.height / (jp2Header.tdy * scale)) + 1;
			var tileX = Math.floor(-xPos / (jp2Header.tdx * scale));
			var tileY = Math.floor(-yPos / (jp2Header.tdy * scale));

			if(tileX < 0) { tileX = 0; }
			if(tileY < 0) { tileY = 0; }

			if(tileX + tilesX > jp2Header.tw)  { tilesX = jp2Header.tw - tileX; }
			if(tileY + tilesY > jp2Header.th)  { tilesY = jp2Header.th - tileY; }

			if(jp2Header.x1 * scale <= canvas.width) { 
				ctx.clearRect(0,0, xPos, canvas.height);
				ctx.clearRect(canvas.width - xPos, 0, xPos, canvas.height);
			}

			if(jp2Header.y1 * scale <= canvas.height) { 
				ctx.clearRect(0, jp2Header.y1 * scale, canvas.width, canvas.height - jp2Header.y1 * scale);
			}

			tiles2go = tilesX * tilesY;
			for(var x = tileX; x < tileX + tilesX; x++) {
				for(var y = tileY; y < tileY + tilesY; y++) {
					var tileIndex = x + (y * jp2Header.tw);
					tiles2go++;

					if(tiles["redux-" + reduction][tileIndex]) {
						tiles["redux-" + reduction][tileIndex].dims = { 
							x: xPos + Math.floor(x * (jp2Header.tdx * scale)), 
							y: yPos + Math.floor(y * (jp2Header.tdy * scale)),
							s: tileS
						};

						drawTile(tiles["redux-" + reduction][tileIndex]); 
					} else {
						var tile = { 
							img: new Image(),
							dims:  { 
								x: xPos + Math.floor(x * (jp2Header.tdx * scale)), 
								y: yPos + Math.floor(y * (jp2Header.tdy * scale)),
								s: tileS
							}
						}
						tiles["redux-" + reduction][tileIndex] = tile;
						tile.img.src = workers[currentWorker].address + "?" + $.param({
							f: filename,
							t: tileIndex,
							r: reduction
						});
						drawTile(tile);
					}

					if(++currentWorker == workers.length) { currentWorker = 0; }
				}
			}
		}

		function initialize(__init) {
			jp2Header = __init || jp2Header ;

			setScale(initScale);
			setReduction();
			if(__init) { initializeTiles(); }

			ensureBounds();
			showImage();
			if(jp2Header.x1 * scale <= canvas.width) { 
				ctx.clearRect(0,0, xPos, canvas.height);
				ctx.clearRect(canvas.width - xPos, 0, xPos, canvas.height);
			}

			if(jp2Header.y1 * scale <= canvas.height) { 
				ctx.clearRect(0, jp2Header.y1 * scale, canvas.width, canvas.height - jp2Header.y1 * scale);
			}

		}



		$.ajax(primary, {
			data: { f: filename },
			dataType: opts.dataType,
			success: initialize
		});
	};
}( jQuery ));

/\.text/ { n = $2; sub(/text/, "ramfunc", n); print "--rename-section "$2"="n; }

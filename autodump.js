// =============================================================
//  Ceitus AutoDump v2 — descarga offsets de theo e inyecta en Offsets.hpp
//  Mejoras sobre v1:
//    - Backup .bak antes de pisar
//    - Resalta pointers críticos (FakeDataModel / VisualEngine / TaskScheduler)
//    - Agrega campos nuevos a namespaces existentes (append antes del `}`)
//    - Agrega namespaces nuevos al final del Offsets {}
//    - Limpia comentarios de versión stale ("// version-xxx") en campos actualizados
//    - Exit code != 0 si alguno de los pointers críticos no se actualizó
//    - 404 → mensaje claro de "theo no dumpeó esta versión todavía"
// =============================================================

const https = require('https');
const fs    = require('fs');
const path  = require('path');
const os    = require('os');

const OFFSETS_HPP = path.join(__dirname, 'RbxESP', 'RbxESP', 'Offsets.hpp');
const THEO_API    = 'https://offsets.imtheo.lol';

// Pointers absolutos — si no entran al exe, el ESP no engancha
const CRITICAL_POINTERS = [
    'FakeDataModel::Pointer',
    'VisualEngine::Pointer',
    'TaskScheduler::Pointer',
];

const NAMESPACE_MAP = {
    'LRUHolder': 'LruHolder',
    'LRUNode':   'LruNode',
};

// null en field  = ignorar ese field puntual
// null en whole ns = ignorar namespace entero (no insertar)
const FIELD_MAP = {
    'ProximityPrompt':      { 'KeyCode': 'KeyboardKeyCode', 'GamepadKeyCode': null },
    'MeshContentProvider':  { 'LRUHolder': 'LruHolder' },
    'SpawnLocation':        { 'Enabled': null, 'Neutral': null, 'TeamColor': null },
    'GuiObject':            { 'ScreenGui_Enabled': null, 'Image': null, 'Text': null, 'RichText': null, 'TextColor3': null },
    'GuiBase2D':            { 'AbsoluteSize': null },
    'Lighting':             { 'GlobalShadows': null },
    'Humanoid':             { 'WalkTimer': null, 'FloorMaterial': null },
    'FileMeshData':         { 'AABBMin': 'AabbMin', 'AABBMax': 'AabbMax' },
    'DragDetector':         { 'ReferenceInstance': null, 'MaxDragTranslation': null, 'MinDragTranslation': null, 'ActivatedCursorIcon': null, 'CursorIcon': null },
    'Player':               { 'CameraMode': null, 'Mouse': null },
    'LocalScript':          { 'GUID': null, 'Hash': null, 'ByteCode': null },
    'Script':               { 'GUID': null, 'Hash': null, 'ByteCode': null },
    'ClickDetector':        { 'MouseIcon': null },
    'Tool':                 { 'TextureId': null },
    'Sound':                { 'SoundGroup': null },
    // namespaces completos a ignorar (ya ignorados en v1):
    'AnimationTrack':  null,
    'StatsItem':       null,
    'PlayerMouse':     null,
    'UnionOperation':  null,
};

// ---------- utilidades ----------
function c(code, s) { return `\x1b[${code}m${s}\x1b[0m`; }
const col = {
    dim:    s => c('2',  s),
    red:    s => c('31', s),
    green:  s => c('32', s),
    yellow: s => c('33', s),
    cyan:   s => c('36', s),
    bold:   s => c('1',  s),
};

function fetchJSON(url, redirects = 0) {
    return new Promise((resolve, reject) => {
        if (redirects > 4) return reject(new Error('demasiados redirects'));
        https.get(url, { headers: { 'User-Agent': 'CeitusAutoDump/2.0' } }, res => {
            if (res.statusCode === 301 || res.statusCode === 302) {
                return fetchJSON(res.headers.location, redirects + 1).then(resolve, reject);
            }
            if (res.statusCode === 404) {
                return reject(new Error('404'));
            }
            if (res.statusCode !== 200) return reject(new Error(`HTTP ${res.statusCode}`));
            let data = '';
            res.on('data', c => data += c);
            res.on('end', () => { try { resolve(JSON.parse(data)); } catch (e) { reject(e); } });
        }).on('error', reject);
    });
}

function getLatestVersion() {
    const versionsDir = path.join(os.homedir(), 'AppData', 'Local', 'Roblox', 'Versions');
    if (!fs.existsSync(versionsDir)) return null;
    const dirs = fs.readdirSync(versionsDir, { withFileTypes: true })
        .filter(d => d.isDirectory() && d.name.startsWith('version-'))
        .map(d => ({ name: d.name, mtime: fs.statSync(path.join(versionsDir, d.name)).mtimeMs }))
        .sort((a, b) => b.mtime - a.mtime);
    return dirs.length > 0 ? dirs[0].name : null;
}

const parseHex = s => parseInt(s, 16);

// locate exact bounds of a namespace block (balanceo de llaves)
function findNamespace(hpp, nsName) {
    const re = new RegExp(`namespace\\s+${nsName}\\s*\\{`, 'g');
    const m = re.exec(hpp);
    if (!m) return null;
    const open = m.index + m[0].length - 1; // posición de la `{`
    let depth = 1, i = open + 1;
    while (i < hpp.length && depth > 0) {
        const ch = hpp[i];
        if (ch === '{') depth++;
        else if (ch === '}') depth--;
        i++;
    }
    if (depth !== 0) return null;
    return { start: m.index, open, close: i - 1, end: i };
}

// ---------- programa ----------
async function main() {
    console.log(col.bold(col.cyan('Ceitus AutoDump v2')));

    let version = process.argv[2];
    if (!version) {
        version = getLatestVersion();
        if (!version) {
            console.error(col.red('No pude detectar la version de Roblox.'));
            console.error('Uso: node autodump.js version-XXXXX');
            process.exit(1);
        }
        console.log(`Version detectada: ${col.cyan(version)}`);
    }
    if (!version.startsWith('version-')) version = 'version-' + version;

    const url = `${THEO_API}/${version}/offsetshex.json`;
    console.log(col.dim(`GET ${url}`));

    let data;
    try {
        data = await fetchJSON(url);
    } catch (e) {
        if (e.message === '404') {
            console.error(col.red(`\n404 — theo todavia no dumpeo ${version}.`));
            console.error('Esperá a que aparezca en https://offsets.imtheo.lol/');
        } else {
            console.error(col.red(`Error: ${e.message}`));
        }
        process.exit(1);
    }

    console.log(col.dim(`Dumper ${data['Dumper Version'] || '?'}  |  ${data['Total Offsets']} offsets  |  ${data['Dumped At']}`));

    const theoOffsets = data.Offsets;
    if (!theoOffsets) { console.error(col.red('JSON sin "Offsets"')); process.exit(1); }
    if (!fs.existsSync(OFFSETS_HPP)) { console.error(col.red(`No existe ${OFFSETS_HPP}`)); process.exit(1); }

    let hpp = fs.readFileSync(OFFSETS_HPP, 'utf8');
    const original = hpp;

    // ---------- backup ----------
    const backupPath = OFFSETS_HPP + '.bak';
    fs.writeFileSync(backupPath, original, 'utf8');
    console.log(col.dim(`backup → ${path.basename(backupPath)}`));

    // ---------- header ----------
    const today = new Date();
    const dateStr = `${String(today.getDate()).padStart(2,'0')}/${String(today.getMonth()+1).padStart(2,'0')}/${today.getFullYear()}`;
    hpp = hpp.replace(/\/\*\s*Roblox Version\s*:.*/, `/*  Roblox Version  : ${version}`);
    hpp = hpp.replace(/\/\*\s*Updated At\s*:.*/,     `/*  Updated At      : ${dateStr}`);
    hpp = hpp.replace(/inline std::string ClientVersion = "[^"]*";/, `inline std::string ClientVersion = "${version}";`);

    // ---------- merge ----------
    let updated = 0, skippedZero = 0, nsNotFound = 0, newFields = 0, newNamespaces = 0;
    const changes = [];
    const critical = {}; // 'FakeDataModel::Pointer' -> {old, new, updated:bool}
    for (const cp of CRITICAL_POINTERS) critical[cp] = { old: null, new: null, updated: false };

    for (const [theoNs, theoFields] of Object.entries(theoOffsets)) {
        if (FIELD_MAP[theoNs] === null) continue;
        const ourNs = NAMESPACE_MAP[theoNs] || theoNs;

        let bounds = findNamespace(hpp, ourNs);

        // crear namespace nuevo si falta
        if (!bounds) {
            nsNotFound++;
            // insertar antes del `}` final del `namespace Offsets {`
            const outerClose = hpp.lastIndexOf('}');
            if (outerClose < 0) continue;
            const lines = [];
            lines.push(``);
            lines.push(`    namespace ${ourNs} {`);
            for (const [fName, fVal] of Object.entries(theoFields)) {
                const fieldMap = FIELD_MAP[theoNs];
                if (fieldMap && fieldMap[fName] === null) continue;
                const ourField = (fieldMap && fieldMap[fName]) || fName;
                const hex = fVal.startsWith('0x') ? fVal : '0x' + fVal;
                if (parseHex(hex.slice(2)) === 0) { skippedZero++; continue; }
                lines.push(`        inline uintptr_t ${ourField} = ${hex.toLowerCase()};`);
                if (CRITICAL_POINTERS.includes(`${ourNs}::${ourField}`)) {
                    critical[`${ourNs}::${ourField}`] = { old: null, new: hex, updated: true };
                }
            }
            lines.push(`    }`);
            hpp = hpp.substring(0, outerClose) + lines.join('\n') + '\n' + hpp.substring(outerClose);
            newNamespaces++;
            changes.push(col.green(`+ namespace ${ourNs}  (nuevo, ${lines.length - 2} fields)`));
            continue;
        }

        // merge por field
        for (const [theoField, theoVal] of Object.entries(theoFields)) {
            const fieldMap = FIELD_MAP[theoNs];
            if (fieldMap && fieldMap[theoField] === null) continue;
            const ourField = (fieldMap && fieldMap[theoField]) || theoField;

            const hexVal = theoVal.startsWith('0x') ? theoVal : '0x' + theoVal;
            if (parseHex(hexVal.slice(2)) === 0) { skippedZero++; continue; }

            // recomputar bounds porque pueden haberse movido tras ediciones previas
            bounds = findNamespace(hpp, ourNs);
            if (!bounds) break;
            const block = hpp.substring(bounds.start, bounds.end);

            const fieldRe = new RegExp(
                `(inline\\s+uintptr_t\\s+${ourField}\\s*=\\s*)(0x[0-9a-fA-F]+)(\\s*;)([^\\n]*)`,
            );
            const fMatch = block.match(fieldRe);
            const newVal = hexVal.toLowerCase();
            const critKey = `${ourNs}::${ourField}`;
            const isCrit  = CRITICAL_POINTERS.includes(critKey);

            if (!fMatch) {
                // field nuevo → insertar antes del `}` del namespace
                const insert = `        inline uintptr_t ${ourField} = ${newVal};\n    `;
                const closeIdx = bounds.close;
                hpp = hpp.substring(0, closeIdx) + insert + hpp.substring(closeIdx);
                newFields++;
                changes.push(col.green(`+ ${ourNs}::${ourField} = ${newVal}`));
                if (isCrit) critical[critKey] = { old: null, new: newVal, updated: true };
                continue;
            }

            const oldVal = fMatch[2].toLowerCase();
            if (isCrit) critical[critKey].old = oldVal;

            if (isCrit) { critical[critKey].old = oldVal; critical[critKey].new = newVal; critical[critKey].updated = true; }

            if (oldVal !== newVal) {
                // reemplazar valor y strip de comentario "// version-xxx" stale
                let trailing = fMatch[4] || '';
                if (/\/\/\s*version-[0-9a-f]+/i.test(trailing)) trailing = '';
                const newBlock = block.replace(fieldRe, `$1${newVal}$3${trailing}`);
                hpp = hpp.substring(0, bounds.start) + newBlock + hpp.substring(bounds.end);
                changes.push(`  ${ourNs}::${ourField}: ${col.dim(oldVal)} → ${col.cyan(newVal)}`);
            }
            updated++;
        }
    }

    // ---------- escribir ----------
    const changed = (hpp !== original);
    if (changed) {
        fs.writeFileSync(OFFSETS_HPP, hpp, 'utf8');
        console.log(col.green(`\nOffsets.hpp actualizado para ${version}`));
    } else {
        console.log(col.dim(`\nSin cambios en Offsets.hpp (ya estaba al dia)`));
        // backup innecesario si no hubo cambios → borrar
        try { fs.unlinkSync(backupPath); } catch (_) {}
    }

    // ---------- resumen ----------
    console.log(`\n${col.bold('Resumen')}`);
    console.log(`  campos pisados:     ${updated}`);
    console.log(`  campos nuevos:      ${newFields}`);
    console.log(`  namespaces nuevos:  ${newNamespaces}`);
    console.log(`  saltados (0x0):     ${skippedZero}`);

    // ---------- pointers críticos ----------
    console.log(`\n${col.bold('Pointers criticos')}`);
    let anyCritFail = false;
    for (const [key, info] of Object.entries(critical)) {
        if (!info.updated) {
            console.log(`  ${col.red('✗')} ${key}  ${col.red('NO SE ACTUALIZO')} — el ESP no va a enganchar`);
            anyCritFail = true;
        } else if (info.old && info.new && info.old !== info.new) {
            console.log(`  ${col.green('✓')} ${key}  ${col.dim(info.old)} → ${col.cyan(info.new)}  ${col.yellow('(RECOMPILAR)')}`);
        } else {
            console.log(`  ${col.dim('·')} ${key}  ${col.cyan(info.new || info.old)}  ${col.dim('(sin cambios)')}`);
        }
    }

    if (changes.length) {
        const maxShow = 40;
        console.log(`\n${col.bold(`Cambios (${changes.length})`)}`);
        for (const c of changes.slice(0, maxShow)) console.log(c);
        if (changes.length > maxShow) console.log(col.dim(`  ... y ${changes.length - maxShow} mas`));
    }

    if (anyCritFail) {
        console.error(col.red('\nAbortando: pointer critico no actualizado. Recompilar seria inutil.'));
        process.exit(2);
    }
}

main().catch(e => { console.error(col.red(e.stack || e.message)); process.exit(1); });

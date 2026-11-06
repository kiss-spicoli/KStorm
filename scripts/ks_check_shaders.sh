#!/usr/bin/env bash
# Validate shaders with glslang the way the viewer compiles them: GLSL 4.10, plus the
# defines LLShaderMgr::loadShaderFile() injects and the permutations set in
# llviewershadermgr.cpp. A shader's #ifdef branches are only checked in the variants
# listed here. Exact OIT and AVBOIT variants are skipped: they need newer GLSL.
#
# Usage: scripts/ks_check_shaders.sh <file.glsl>...
# Set GLSLANG to the validator's path if it is not glslangValidator on the PATH.

GLSLANG="${GLSLANG:-glslangValidator}"
tmp="${RUNNER_TEMP:-$(mktemp -d)}"
fail=0

# check <file> <defines...>; NAME or NAME=VALUE. Errors report the source file's own lines.
check() {
    local f="$1" ext stage out d
    shift
    case "$f" in
        *F.glsl) ext=frag; stage=FRAGMENT_SHADER ;;
        *V.glsl) ext=vert; stage=VERTEX_SHADER ;;
        *) return ;;
    esac
    out="$tmp/check.$ext"
    {
        echo "#version 410 core"
        echo "#define $stage 1"
        echo "#define GBUFFER_FLAG_SKIP_ATMOS   0.0"
        echo "#define GBUFFER_FLAG_HAS_ATMOS    0.34"
        echo "#define GBUFFER_FLAG_HAS_PBR      0.67"
        echo "#define GBUFFER_FLAG_HAS_HDRI      1.0"
        echo "#define GET_GBUFFER_FLAG(data, flag)    (abs(data-flag)< 0.1)"
        # Injected into every shader by LLShaderMgr::loadShaderFile() (master copy in deferredUtil.glsl).
        echo "struct GBufferInfo { vec4 albedo; vec4 specular; vec3 normal; vec4 emissive; float gbufferFlag; float envIntensity; };"
        for d in "$@"; do
            echo "#define ${d%%=*} $([ "$d" != "${d#*=}" ] && echo "${d#*=}" || echo 1)"
            # The viewer generates diffuseLookup() for indexed textures; declaring it is enough here.
            [ "$d" = USE_INDEXED_TEX ] && echo "vec4 diffuseLookup(vec2 texcoord);"
        done
        echo "#line 1"
        cat "$f"
    } > "$out"
    echo "== $f [$*]"
    if ! "$GLSLANG" "$out"; then
        echo "::error file=$f::GLSL errors in variant [$*]"
        fail=1
    fi
}

for f in "$@"; do
    case "$f" in
        */gltf/*)
            # The viewer sets these from the GPU's uniform block size (llviewershadermgr.cpp);
            # here they are OpenGL's guaranteed minimum block size, 16 KB.
            check "$f" MAX_UBO_VEC4S=1024 MAX_NODES_PER_GLTF_OBJECT=341 MAX_MATERIALS_PER_GLTF_OBJECT=85
            continue
            ;;
    esac
    case "${f##*/}" in
        alphaF.glsl)
            check "$f" USE_VERTEX_COLOR HAS_ALPHA_MASK USE_INDEXED_TEX HAS_SUN_SHADOW
            check "$f" USE_VERTEX_COLOR HAS_ALPHA_MASK USE_INDEXED_TEX IS_HUD
            check "$f" USE_VERTEX_COLOR HAS_ALPHA_MASK USE_INDEXED_TEX FOR_IMPOSTOR
            check "$f" USE_DIFFUSE_TEX IS_AVATAR_SKIN HAS_SUN_SHADOW
            ;;
        materialF.glsl)
            check "$f" DIFFUSE_ALPHA_MODE=0 HAS_NORMAL_MAP HAS_SPECULAR_MAP
            check "$f" DIFFUSE_ALPHA_MODE=1 HAS_ALPHA_MASK HAS_NORMAL_MAP HAS_SPECULAR_MAP HAS_SUN_SHADOW
            check "$f" DIFFUSE_ALPHA_MODE=1 HAS_ALPHA_MASK
            check "$f" DIFFUSE_ALPHA_MODE=2 HAS_ALPHA_MASK HAS_SUN_SHADOW
            check "$f" DIFFUSE_ALPHA_MODE=3 HAS_ALPHA_MASK
            ;;
        pbralphaF.glsl)
            check "$f" HAS_NORMAL_MAP HAS_SPECULAR_MAP HAS_EMISSIVE_MAP USE_VERTEX_COLOR HAS_SUN_SHADOW
            check "$f" HAS_NORMAL_MAP HAS_SPECULAR_MAP HAS_EMISSIVE_MAP USE_VERTEX_COLOR IS_HUD
            ;;
        *)
            check "$f"
            ;;
    esac
done
exit "$fail"

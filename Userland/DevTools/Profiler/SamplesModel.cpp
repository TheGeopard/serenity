/*
 * Copyright (c) 2018-2021, Andreas Kling <kling@serenityos.org>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "SamplesModel.h"
#include "Profile.h"
#include <AK/StringBuilder.h>
#include <stdio.h>

SamplesModel::SamplesModel(Profile& profile)
    : m_profile(profile)
{
    m_user_frame_icon.set_bitmap_for_size(16, Gfx::Bitmap::load_from_file("/res/icons/16x16/inspector-object.png"));
    m_kernel_frame_icon.set_bitmap_for_size(16, Gfx::Bitmap::load_from_file("/res/icons/16x16/inspector-object-red.png"));
}

SamplesModel::~SamplesModel()
{
}

int SamplesModel::row_count(const GUI::ModelIndex&) const
{
    return m_profile.filtered_event_count();
}

int SamplesModel::column_count(const GUI::ModelIndex&) const
{
    return Column::__Count;
}

String SamplesModel::column_name(int column) const
{
    switch (column) {
    case Column::SampleIndex:
        return "#";
    case Column::Timestamp:
        return "Timestamp";
    case Column::ThreadID:
        return "TID";
    case Column::ExecutableName:
        return "Executable";
    case Column::InnermostStackFrame:
        return "Innermost Frame";
    default:
        VERIFY_NOT_REACHED();
    }
}

GUI::Variant SamplesModel::data(const GUI::ModelIndex& index, GUI::ModelRole role) const
{
    u32 event_index = m_profile.first_filtered_event_index() + index.row();
    auto& event = m_profile.events().at(event_index);

    if (role == GUI::ModelRole::Custom) {
        return event_index;
    }

    if (role == GUI::ModelRole::Display) {
        if (index.column() == Column::SampleIndex)
            return event_index;

        if (index.column() == Column::ThreadID)
            return event.tid;

        if (index.column() == Column::ExecutableName) {
            // FIXME: More abuse of the PID/TID relationship:
            if (auto* process = m_profile.find_process(event.tid))
                return process->executable;
            return "";
        }

        if (index.column() == Column::Timestamp) {
            return (u32)event.timestamp;
        }

        if (index.column() == Column::InnermostStackFrame) {
            return event.frames.last().symbol;
        }
        return {};
    }
    return {};
}

void SamplesModel::update()
{
    did_update(Model::InvalidateAllIndexes);
}
